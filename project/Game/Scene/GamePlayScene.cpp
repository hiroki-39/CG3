#define NOMINMAX
#include "GamePlayScene.h"
#include "KHEngine/Core/Services/EngineServices.h"
#include "KHEngine/Core/Utility/Log/Logger.h"
#include "KHEngine/Graphics/3d/Model/ModelManager.h"
#include "KHEngine/Graphics/Resource/Texture/TextureManager.h"
#include "KHEngine/Graphics/3d/Particle/ParticleManager.h"
#include "KHEngine/Graphics/Billboard/Billboard.h"
#include "KHEngine/Debug/Imgui/ImGuiManager.h"
#include "KHEngine/Graphics/3d/Particle/ParticleRenderer.h"
#include "KHEngine/Sound/Core/SoundManager.h"
#include <algorithm>
#include <random>
#include <memory>
#include <cmath>
#include <numbers>
#include "KHEngine/Scene/LevelLoader.h"
#include "KHEngine/Graphics/Resource/Texture/TextureManager.h"
#include "KHEngine/Core/Resource/ResourceLocator.h"
#include "externals/imgui/imgui.h"
#include <filesystem>
#include "KHEngine/Math/CollisionMath.h"

static void CreateObjectFromNode(const LevelObjectData& node, const Object3d* parentObj, std::vector<std::unique_ptr<Object3d>>& instances, std::vector<std::unique_ptr<Rail>>& outRails, Object3dCommon* common, uint32_t skyboxTexIndex, std::list<std::unique_ptr<Enemy>>& enemies, std::list<std::unique_ptr<Obstacle>>& obstacles, std::vector<Enemy*> parentEnemies = {})
{
    const Object3d* currentObj = parentObj;

    if (node.type == "CURVE")
    {
        // Blenderのエクスポータですでにワールド座標としてcurvePointsが出力されているため、
        // ゲーム側での再変換は不要。そのままレールを初期化する。
        if (!parentEnemies.empty())
        {
            for (Enemy* e : parentEnemies)
            {
                auto rail = std::make_unique<Rail>();
                rail->Initialize(node.curvePoints);
                e->SetMovePath(std::move(rail));
            }
        }
        else
        {
            auto rail = std::make_unique<Rail>();
            rail->Initialize(node.curvePoints);
            outRails.push_back(std::move(rail));
        }
    }

    std::vector<Enemy*> currentEnemies = parentEnemies;
    // オブジェクトの種別判定
    bool isObstacle = (node.fileName.find("Obstacle") != std::string::npos) || (node.fileName.find("Invisible") != std::string::npos) || (node.fileName.find("ColliderOnly") != std::string::npos);
    bool isEnemy = (node.fileName.find("Fighter") != std::string::npos || node.fileName.find("Asteroid") != std::string::npos || node.fileName.find("Enemy") != std::string::npos);

    if (isObstacle)
    {
        // Obstacleとして生成
        auto obstacle = std::make_unique<Obstacle>();
        // Blenderのrotationは度数法だがObstacleはInitializeでそのまま保持するためラジアン変換する
        Vector3 rotRad;
        rotRad.x = node.rotation.x * (std::numbers::pi_v<float> / 180.0f);
        rotRad.y = node.rotation.y * (std::numbers::pi_v<float> / 180.0f);
        rotRad.z = node.rotation.z * (std::numbers::pi_v<float> / 180.0f);
        obstacle->Initialize(common, node.translation, node.scale, rotRad, node.fileName, skyboxTexIndex, node.collider, node.isDestructible);
        obstacle->SetSpawnProgress(node.spawnProgress);
        if (!node.texturePath.empty())
        {
            obstacle->SetTexturePath(node.texturePath);
        }
        obstacles.push_back(std::move(obstacle));
    }
    else if (isEnemy)
    {
        currentEnemies.clear();
        int count = std::max<int>(1, node.spawnCount);
        for (int i = 0; i < count; ++i)
        {
            auto enemy = std::make_unique<Enemy>();

            // 陣形によるオフセット計算
            Vector3 offset = { 0, 0, 0 };
            if (node.formationType == "LINE")
            {
                offset.z = i * node.formationSpacing;
            }
            else if (node.formationType == "V_SHAPE")
            {
                if (i > 0)
                {
                    float side = (i % 2 == 1) ? 1.0f : -1.0f;
                    int row = (i + 1) / 2;
                    offset.x = side * row * node.formationSpacing;
                    offset.z = row * node.formationSpacing;
                }
            }
            else if (node.formationType == "HORIZONTAL")
            {
                if (i > 0)
                {
                    float side = (i % 2 == 1) ? 1.0f : -1.0f;
                    int row = (i + 1) / 2;
                    offset.x = side * row * node.formationSpacing;
                }
            }

            // オフセットを適用した新しいノードデータを作成して初期化
            LevelObjectData spawnNode = node;
            spawnNode.translation.x += offset.x;
            spawnNode.translation.y += offset.y;
            spawnNode.translation.z += offset.z;

            enemy->Initialize(common, spawnNode, skyboxTexIndex);
            enemy->SetSpawnProgress(node.spawnProgress);
            enemy->SetSpawnDelay(i * node.spawnInterval); // 時間差出現

            if (!node.texturePath.empty())
            {
                enemy->SetTexturePath(node.texturePath);
            }

            // パスの引き継ぎ（親ではなく、スポナーが持っているパスを全敵に渡す）
            currentEnemies.push_back(enemy.get());

            enemies.push_back(std::move(enemy));
        }
    }
    else if (node.type == "MESH")
    {
        auto obj = std::make_unique<Object3d>();
        obj->Initialize(common);

        std::string modelName = node.fileName;
        if (modelName.empty())
        {
            modelName = node.name + ".obj";
        }

        // ロードされていない可能性を考慮
        ModelManager::GetInstance()->LoadModel(modelName);
        if (ModelManager::GetInstance()->FindModel(modelName) != nullptr)
        {
            obj->SetModel(modelName);
        }

        obj->SetTranslate(node.translation);

        // Blender出力は恐らく度数法(Degree)なのでラジアンに変換
        Vector3 rotRad;
        rotRad.x = node.rotation.x * (std::numbers::pi_v<float> / 180.0f);
        rotRad.y = node.rotation.y * (std::numbers::pi_v<float> / 180.0f);
        rotRad.z = node.rotation.z * (std::numbers::pi_v<float> / 180.0f);
        obj->SetRotation(rotRad);

        obj->SetScale(node.scale);
        obj->SetEnvironmentTextureIndex(skyboxTexIndex);

        if (parentObj)
        {
            obj->SetParent(parentObj);
        }

        currentObj = obj.get();
        instances.push_back(std::move(obj));
    }

    // 子オブジェクトを再帰的に生成
    for (const auto& child : node.children)
    {
        CreateObjectFromNode(child, currentObj, instances, outRails, common, skyboxTexIndex, enemies, obstacles, currentEnemies);
    }
}

static void LoadEnemiesOnlyFromNode(const LevelObjectData& node, Object3dCommon* common, uint32_t skyboxTexIndex, std::list<std::unique_ptr<Enemy>>& enemies, std::list<std::unique_ptr<Obstacle>>& obstacles, std::vector<Enemy*> parentEnemies = {})
{
    if (node.type == "CURVE" && !parentEnemies.empty())
    {
        for (Enemy* e : parentEnemies)
        {
            auto rail = std::make_unique<Rail>();
            rail->Initialize(node.curvePoints);
            e->SetMovePath(std::move(rail));
        }
    }

    std::vector<Enemy*> currentEnemies = parentEnemies;

    bool isObstacle = (node.fileName.find("Obstacle") != std::string::npos) || (node.fileName.find("Invisible") != std::string::npos) || (node.fileName.find("ColliderOnly") != std::string::npos);
    bool isEnemy = (node.fileName.find("Fighter") != std::string::npos || node.fileName.find("Asteroid") != std::string::npos || node.fileName.find("Enemy") != std::string::npos);

    if (isObstacle)
    {
        auto obstacle = std::make_unique<Obstacle>();
        Vector3 rotRad;
        rotRad.x = node.rotation.x * (std::numbers::pi_v<float> / 180.0f);
        rotRad.y = node.rotation.y * (std::numbers::pi_v<float> / 180.0f);
        rotRad.z = node.rotation.z * (std::numbers::pi_v<float> / 180.0f);
        obstacle->Initialize(common, node.translation, node.scale, rotRad, node.fileName, skyboxTexIndex, node.collider, node.isDestructible);
        obstacle->SetSpawnProgress(node.spawnProgress);
        if (!node.texturePath.empty())
        {
            obstacle->SetTexturePath(node.texturePath);
        }
        obstacles.push_back(std::move(obstacle));
    }
    else if (isEnemy)
    {
        currentEnemies.clear();
        int count = std::max<int>(1, node.spawnCount);
        for (int i = 0; i < count; ++i)
        {
            auto enemy = std::make_unique<Enemy>();

            // 陣形によるオフセット計算
            Vector3 offset = { 0, 0, 0 };
            if (node.formationType == "LINE")
            {
                offset.z = i * node.formationSpacing;
            }
            else if (node.formationType == "V_SHAPE")
            {
                if (i > 0)
                {
                    float side = (i % 2 == 1) ? 1.0f : -1.0f;
                    int row = (i + 1) / 2;
                    offset.x = side * row * node.formationSpacing;
                    offset.z = row * node.formationSpacing;
                }
            }
            else if (node.formationType == "HORIZONTAL")
            {
                if (i > 0)
                {
                    float side = (i % 2 == 1) ? 1.0f : -1.0f;
                    int row = (i + 1) / 2;
                    offset.x = side * row * node.formationSpacing;
                }
            }

            // オフセットを適用した新しいノードデータを作成して初期化
            LevelObjectData spawnNode = node;
            spawnNode.translation.x += offset.x;
            spawnNode.translation.y += offset.y;
            spawnNode.translation.z += offset.z;

            enemy->Initialize(common, spawnNode, skyboxTexIndex);
            enemy->SetSpawnProgress(node.spawnProgress);
            enemy->SetSpawnDelay(i * node.spawnInterval); // 時間差出現

            if (!node.texturePath.empty())
            {
                enemy->SetTexturePath(node.texturePath);
            }
            currentEnemies.push_back(enemy.get());
            enemies.push_back(std::move(enemy));
        }
    }

    for (const auto& child : node.children)
    {
        LoadEnemiesOnlyFromNode(child, common, skyboxTexIndex, enemies, obstacles, currentEnemies);
    }
}

void GamePlayScene::Initialize()
{
    // フレームワーク共通オブジェクトを取得
    auto services = EngineServices::GetInstance();
    auto object3dCommon = services->GetObject3dCommon();
    auto dxCommon = services->GetDirectXCommon();
    auto srvManager = services->GetSrvManager();
    auto spriteCommon = services->GetSpriteCommon();

    // カメラ作成（ゲーム固有）
    camera = std::make_unique<Camera>();
    camera->SetTranslate({ 0.0f, 6.0f, -20.0f });
    camera->SetRotation({ 0.0f, 0.0f, 0.0f }); // 回転を0度にする

    // デバッグカメラ作成
    debugCamera_ = std::make_unique<Camera>();
    debugCamera_->SetTranslate({ 0.0f, 6.0f, -20.0f });
    debugCamera_->SetRotation({ 0.0f, 0.0f, 0.0f });

    activeCamera_ = camera.get();

    if (object3dCommon)
    {
        object3dCommon->SetDefaultCamera(activeCamera_);
    }

    // アセット登録
    ParticleManager::GetInstance()->RegisterQuad("quad", "resources/circle.png");
    ParticleManager::GetInstance()->RegisterRing("ring", "gradationLine.png", 32, 0.5f, 1.0f);
    ParticleManager::GetInstance()->RegisterCylinder("Cylinder", "resources/sprites/gradationLine.png");

    uint32_t instancingSrvIndex = UINT32_MAX;

    auto texManager = TextureManager::GetInstance();
    dxCommon->BeginTextureUploadBatch();

    // スカイボックスの初期化
    skybox_ = std::make_unique<Skybox>();
    skybox_->Initialize(dxCommon, "resources/skybox.dds");

    // モデル読み込み
    ModelManager::GetInstance()->LoadModel("plane.obj");
    ModelManager::GetInstance()->LoadModel("Cube.obj");
    ModelManager::GetInstance()->LoadModel("cube.obj");
    ModelManager::GetInstance()->LoadModel("monsterBall.obj");
    ModelManager::GetInstance()->LoadModel("terrain.obj");
    ModelManager::GetInstance()->LoadModel("player.obj");
    ModelManager::GetInstance()->LoadModel("suzanne.obj");

    // スプライト用テクスチャ読み込み
    texManager->LoadTexture("uvChecker.png");
    texManager->LoadTexture("monsterBall.png");
    texManager->LoadTexture("checkerBoard.png");
    texManager->LoadTexture("resources/skybox.dds");
    texManager->LoadTexture("circle.png");
    texManager->LoadTexture("circle2.png");
    texManager->LoadTexture("gradationLine.png");
    texManager->LoadTexture("white.png");


    uint32_t uvCheckerTex = TextureManager::GetInstance()->GetTextureIndexByFilePath("uvChecker.png");
    uint32_t monsterBallTex = TextureManager::GetInstance()->GetTextureIndexByFilePath("monsterBall.png");
    uint32_t checkerBoardTex = TextureManager::GetInstance()->GetTextureIndexByFilePath("checkerBoard.png");
    uint32_t skyboxTexIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath("resources/skybox.dds");


    // スプライト作成
    {
        auto s = std::make_unique<Sprite>();
        s->Initialize(spriteCommon, uvCheckerTex);
        s->SetPosition(Vector2(100.0f, 100.0f));
        s->SetSize(Vector2(128.0f, 128.0f));
        s->SetAnchorPoint(Vector2(0.5f, 0.5f));
        s->SetColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
        AddSprite(std::move(s));
    }

    // モデル作成
    {
        //auto obj = std::make_unique<Object3d>();
        //obj->Initialize(object3dCommon);
        //obj->SetModel("suzanne.obj");
        //obj->GetModel()->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
        //uint32_t skyboxTexIndex = skybox_->GetCubemapSrvIndex();
        //obj->SetEnvironmentTextureIndex(skyboxTexIndex);
        //obj->SetEnvironmentCoefficient(1.0f);
        //obj->SetTranslate(Vector3(0.0f, 3.0f, 40.0f));
        //obj->SetRotation(Vector3(0.0f, 2.3f, 0.0f));
        //obj->SetScale(Vector3(3.0f, 3.0f, 3.0f));
        //modelInstances.push_back(std::move(obj));

        auto terrain = std::make_unique<Object3d>();
        terrain->Initialize(object3dCommon);
        terrain->SetModel("terrain.obj");
        terrain->SetTranslate(Vector3(0.0f, -3.0f, 0.0f));
        terrain->SetRotation(Vector3(0.0f, 0.0f, 0.0f));
        terrain->SetScale(Vector3(1.0f, 1.0f, 1.0f));
        modelInstances.push_back(std::move(terrain));
    }

    // ホットリロード用の関数に処理を切り出したので呼び出す
    ReloadLevel();

    // カメラオブジェクト（プレイヤーの親となる）の初期化
    cameraObject_ = std::make_unique<Object3d>();
    cameraObject_->Initialize(object3dCommon);

    // プレイヤーの初期化
    player_ = std::make_unique<Player>();
    player_->Initialize(object3dCommon, skybox_->GetCubemapSrvIndex());
    player_->LoadSettings("resources/json/player/player_settings.json");

    // プレイヤーをカメラオブジェクトの子にする
    player_->GetObject3d()->SetParent(cameraObject_.get());
    if (player_->GetColliderObject())
    {
        player_->GetColliderObject()->SetParent(cameraObject_.get());
    }
    player_->GetReticle()->SetParent(cameraObject_.get());
    if (player_->GetFrontReticle())
    {
        player_->GetFrontReticle()->SetParent(cameraObject_.get());
    }

    // レールカメラコントローラの初期化
    railCameraController_ = std::make_unique<RailCameraController>();
    if (!mainRails_.empty())
    {
        std::vector<Rail*> railsRaw;
        for (auto& r : mainRails_)
        {
            railsRaw.push_back(r.get());
        }
        railCameraController_->Initialize(railsRaw, activeCamera_, cameraObject_.get());
    }

    thrusterEffect_.Initialize(dxCommon, srvManager);
    thrusterEffect_.LoadFromJson("thruster.json");

    explosionEffect_.Initialize(dxCommon, srvManager);
    explosionEffect_.LoadFromJson("explosion.json");

    hitEffect_.Initialize(dxCommon, srvManager);
    hitEffect_.LoadFromJson("hit.json");

    // 回避時の衝撃波（リング）エフェクトの初期化
    dodgeEffect_.Initialize(dxCommon, srvManager);
    dodgeEffect_.LoadFromJson("dodge.json");

    // 全てのモデル・テクスチャ読み込みが終わった後にGPUへ転送する
    texManager->ExecuteUploadCommands();
    texManager->ClearIntermediateResources();
}

void GamePlayScene::ReloadLevel()
{
    auto services = EngineServices::GetInstance();
    auto object3dCommon = services->GetObject3dCommon();

    // 既存データのクリア (先頭のterrainは残す)
    while (modelInstances.size() > 1)
    {
        modelInstances.pop_back();
    }

    enemies_.clear();
    obstacles_.clear();
    bullets_.clear();
    railVisualizers_.clear();
    enemyRailVisualizers_.clear();
    mainRails_.clear();
    if (railCameraController_)
    {
        railCameraController_->Reset();
    }

    // レベルデータの読み込みと配置
    auto levelData = LevelLoader::Load("resources/json/maps/template/template.json");
    if (levelData)
    {
        for (const auto& objData : levelData->objects)
        {
            CreateObjectFromNode(objData, nullptr, modelInstances, mainRails_, object3dCommon, skybox_->GetCubemapSrvIndex(), enemies_, obstacles_);
        }
        OutputDebugStringA("LevelLoader: Successfully reloaded objects.\n");

        // 再初期化
        if (railCameraController_ && !mainRails_.empty())
        {
            std::vector<Rail*> railsRaw;
            for (auto& r : mainRails_)
            {
                railsRaw.push_back(r.get());
            }
            railCameraController_->Initialize(railsRaw, activeCamera_, cameraObject_.get());
        }
    }
    else
    {
        OutputDebugStringA("LevelLoader: Failed to reload level.\n");
    }

    // レールの可視化用オブジェクトの生成
    railVisualizers_.clear();
    railModels_.clear();
    if (!mainRails_.empty())
    {
        std::vector<Vector4> colors = {
            {1.0f, 0.0f, 0.0f, 1.0f}, // 赤
            {0.0f, 0.5f, 1.0f, 1.0f}, // 青
            {1.0f, 1.0f, 0.0f, 1.0f}, // 黄色
            {0.0f, 1.0f, 1.0f, 1.0f}, // シアン
            {1.0f, 0.0f, 1.0f, 1.0f}, // マゼンタ
            {1.0f, 0.5f, 0.0f, 1.0f}  // オレンジ
        };

        std::string resolved = ResourceLocator::Resolve("rail.obj", ResourceLocator::AssetType::Model3D);
        std::string directory = "resources/models";
        std::string filename = "rail.obj";
        if (!resolved.empty())
        {
            std::filesystem::path rp(reinterpret_cast<const char8_t*>(resolved.c_str()));
            directory = rp.parent_path().string();
            filename = rp.filename().string();
        }

        int sampleCount = 200; // 分割数を増やして滑らかな線にする
        int colorIndex = 0;
        for (auto& rail : mainRails_)
        {
            if (!rail || !rail->IsValid()) continue;

            auto railModel = std::make_unique<Model>();
            railModel->Initialize(ModelManager::GetInstance()->GetModelCommon(), directory, filename);
            railModel->SetColor(colors[colorIndex % colors.size()]);

            for (int i = 0; i < sampleCount; ++i)
            {
                float t1 = static_cast<float>(i) / sampleCount;
                float t2 = static_cast<float>(i + 1) / sampleCount;

                Vector3 p1 = rail->GetPosition(t1);
                Vector3 p2 = rail->GetPosition(t2);

                Vector3 dir = { p2.x - p1.x, p2.y - p1.y, p2.z - p1.z };
                float length = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
                if (length < 0.0001f) continue;

                dir.x /= length; dir.y /= length; dir.z /= length;

                Vector3 center = { (p1.x + p2.x) * 0.5f, (p1.y + p2.y) * 0.5f, (p1.z + p2.z) * 0.5f };

                auto obj = std::make_unique<Object3d>();
                obj->Initialize(object3dCommon);
                obj->SetModel(railModel.get());
                obj->SetTranslate(center);
                float yaw = std::atan2(dir.x, dir.z);
                float pitch = std::asin(-dir.y);
                obj->SetRotation(Vector3(pitch, yaw, 0.0f));

                obj->SetScale(Vector3(0.02f, 0.02f, length));

                obj->SetEnvironmentCoefficient(0.0f);
                obj->SetEnvironmentTextureIndex(skybox_->GetCubemapSrvIndex());

                railVisualizers_.push_back(std::move(obj));
            }

            railModels_.push_back(std::move(railModel));
            colorIndex++;
        }
    }

    // 敵用レールの可視化
    enemyRailModel_ = std::make_unique<Model>();
    std::string resolved = ResourceLocator::Resolve("rail.obj", ResourceLocator::AssetType::Model3D);
    if (!resolved.empty())
    {
        std::filesystem::path rp(reinterpret_cast<const char8_t*>(resolved.c_str()));
        enemyRailModel_->Initialize(ModelManager::GetInstance()->GetModelCommon(), rp.parent_path().string(), rp.filename().string());
    }
    else
    {
        enemyRailModel_->Initialize(ModelManager::GetInstance()->GetModelCommon(), "resources/models", "rail.obj");
    }
    enemyRailModel_->SetColor({ 0.5f, 1.0f, 0.0f, 1.0f }); // 黄緑色

    int sampleCount = 100; // 敵用レールは少し分割数を減らす
    for (auto& enemy : enemies_)
    {
        const Rail* rail = enemy->GetMovePath();
        if (!rail || !rail->IsValid()) continue;
        for (int i = 0; i < sampleCount; ++i)
        {
            float t1 = static_cast<float>(i) / sampleCount;
            float t2 = static_cast<float>(i + 1) / sampleCount;

            Vector3 p1 = rail->GetPosition(t1);
            Vector3 p2 = rail->GetPosition(t2);

            Vector3 dir = { p2.x - p1.x, p2.y - p1.y, p2.z - p1.z };
            float length = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
            if (length < 0.0001f) continue;

            dir.x /= length; dir.y /= length; dir.z /= length;

            Vector3 center = { (p1.x + p2.x) * 0.5f, (p1.y + p2.y) * 0.5f, (p1.z + p2.z) * 0.5f };

            auto obj = std::make_unique<Object3d>();
            obj->Initialize(object3dCommon);
            obj->SetModel(enemyRailModel_.get());
            obj->SetTranslate(center);
            float yaw = std::atan2(dir.x, dir.z);
            float pitch = std::asin(-dir.y);
            obj->SetRotation(Vector3(pitch, yaw, 0.0f));

            obj->SetScale(Vector3(0.015f, 0.015f, length)); // メインより少し細く

            obj->SetEnvironmentCoefficient(0.0f);
            obj->SetEnvironmentTextureIndex(skybox_->GetCubemapSrvIndex());

            enemyRailVisualizers_.push_back(std::move(obj));
        }
    }
}

void GamePlayScene::ReloadEnemiesOnly()
{
    auto services = EngineServices::GetInstance();
    auto object3dCommon = services->GetObject3dCommon();

    enemies_.clear();
    obstacles_.clear();
    enemyRailVisualizers_.clear();

    auto levelData = LevelLoader::Load("resources/json/maps/template/template.json");
    if (levelData)
    {
        for (const auto& objData : levelData->objects)
        {
            LoadEnemiesOnlyFromNode(objData, object3dCommon, skybox_->GetCubemapSrvIndex(), enemies_, obstacles_);
        }
        OutputDebugStringA("LevelLoader: Successfully respawned enemies.\n");

        // 敵用レールの可視化再構築
        if (enemyRailModel_)
        {
            int sampleCount = 100;
            for (auto& enemy : enemies_)
            {
                const Rail* rail = enemy->GetMovePath();
                if (!rail || !rail->IsValid()) continue;
                for (int i = 0; i < sampleCount; ++i)
                {
                    float t1 = static_cast<float>(i) / sampleCount;
                    float t2 = static_cast<float>(i + 1) / sampleCount;
                    Vector3 p1 = rail->GetPosition(t1);
                    Vector3 p2 = rail->GetPosition(t2);
                    Vector3 dir = { p2.x - p1.x, p2.y - p1.y, p2.z - p1.z };
                    float length = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
                    if (length < 0.0001f) continue;
                    dir.x /= length; dir.y /= length; dir.z /= length;
                    Vector3 center = { (p1.x + p2.x) * 0.5f, (p1.y + p2.y) * 0.5f, (p1.z + p2.z) * 0.5f };
                    auto obj = std::make_unique<Object3d>();
                    obj->Initialize(object3dCommon);
                    obj->SetModel(enemyRailModel_.get());
                    obj->SetTranslate(center);
                    float yaw = std::atan2(dir.x, dir.z);
                    float pitch = std::asin(-dir.y);
                    obj->SetRotation(Vector3(pitch, yaw, 0.0f));
                    obj->SetScale(Vector3(0.015f, 0.015f, length));
                    obj->SetEnvironmentCoefficient(0.0f);
                    obj->SetEnvironmentTextureIndex(skybox_->GetCubemapSrvIndex());
                    enemyRailVisualizers_.push_back(std::move(obj));
                }
            }
        }
    }
    else
    {
        OutputDebugStringA("LevelLoader: Failed to respawn enemies.\n");
    }
}


void GamePlayScene::Finalize()
{
    // unique_ptr 管理なので明示的な delete は不要
    sprites.clear();
    modelInstances.clear();

    skybox_.reset();
    camera.reset();
    debugCamera_.reset();
    activeCamera_ = nullptr;
}

namespace
{
    float LerpAngle(float a, float b, float t)
    {
        float diff = b - a;
        while (diff < -3.14159265f) diff += 6.2831853f;
        while (diff > 3.14159265f) diff -= 6.2831853f;
        return a + diff * t;
    }
}

void GamePlayScene::Update()
{
    auto services = EngineServices::GetInstance();
    auto input = services->GetInput();
    float dt = services->GetDeltaTime();

    // 0キーでUI表示（エディターモード）に戻す
    if (input && input->TriggerKey(DIK_0))
    {
        services->SetEditorMode(true);
    }

    // ホットリロードのトリガー (F5キー)
    if (input && input->TriggerKey(DIK_F5))
    {
        ReloadLevel();
    }

    // --- カメラ切り替え ---
    if (isPlaying_)
    {
        activeCamera_ = camera.get();
    }
    else
    {
        activeCamera_ = debugCamera_.get();
    }
    if (auto objCommon = services->GetObject3dCommon())
    {
        objCommon->SetDefaultCamera(activeCamera_);
    }

    // --- カメラ操作 ---
    // ・ホイール押し込み（ミドルボタン）を押しながら移動 -> カメラ回転（yaw/pitch）(継続)
    // ・WASDキー -> カメラ移動（カメラの向きに沿った前後左右）
    // ・ホイール回転 -> ズーム (継続)
    if (input && activeCamera_)
    {
        // 感度設定（必要に応じて調整）
        const float kRotateSpeed = 0.005f; // 回転感度（ラジアン換算想定）
        const float kMoveSpeed = 8.0f;     // 移動速度（ワールド単位 / 秒）
        const float kZoomSpeed = 0.0015f;  // ホイール感度（調整可）

        LONG dx = input->GetMouseMoveX();
        LONG dy = input->GetMouseMoveY();
        LONG wheel = input->GetMouseWheel();

        // 停止中のみ自由なカメラ操作を許可
        if (!isPlaying_)
        {
            if (input->PushMouseButton(1)) // 右クリックドラッグで回転
            {
                Vector3 rot = activeCamera_->GetRotation();
                // マウス右移動で yaw 増加、下移動で pitch 増加
                rot.y += static_cast<float>(dx) * kRotateSpeed;
                rot.x += static_cast<float>(dy) * kRotateSpeed;

                // ピッチ（X軸回転）を適度に制限
                const float kMaxPitch = 1.5f;  // 約 85度
                const float kMinPitch = -1.5f; // 約 -85度
                rot.x = std::clamp(rot.x, kMinPitch, kMaxPitch);

                activeCamera_->SetRotation(rot);
            }

            // WASDキーでカメラ移動（カメラのyawに沿った前後左右）
            float currentMoveSpeed = kMoveSpeed;
            if (input->PushKey(DIK_LSHIFT) || input->PushKey(DIK_RSHIFT))
            {
                currentMoveSpeed *= 25.0f; // Shiftキーで超高速移動
            }
            float moveStep = currentMoveSpeed * dt;
            Vector3 pos = activeCamera_->GetTranslate();
            Vector3 rot = activeCamera_->GetRotation();
            float yaw = rot.y;

            // カメラの向きから forward / right を構成
            Vector3 forward = { std::sinf(yaw), 0.0f, std::cosf(yaw) };
            Vector3 right = { std::cosf(yaw), 0.0f, -std::sinf(yaw) };

            auto normalize = [](Vector3 v)
                {
                    float len = std::sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
                    if (len > 1e-6f) { v.x /= len; v.y /= len; v.z /= len; }
                    return v;
                };

            forward = normalize(forward);
            right = normalize(right);

            if (input->PushKey(DIK_W))
            {
                pos.x += forward.x * moveStep;
                pos.z += forward.z * moveStep;
            }
            if (input->PushKey(DIK_S))
            {
                pos.x -= forward.x * moveStep;
                pos.z -= forward.z * moveStep;
            }
            if (input->PushKey(DIK_D))
            {
                pos.x += right.x * moveStep;
                pos.z += right.z * moveStep;
            }
            if (input->PushKey(DIK_A))
            {
                pos.x -= right.x * moveStep;
                pos.z -= right.z * moveStep;
            }
            if (input->PushKey(DIK_E)) // 上昇
            {
                pos.y += moveStep;
            }
            if (input->PushKey(DIK_Q)) // 下降
            {
                pos.y -= moveStep;
            }

            activeCamera_->SetTranslate(pos);

            // 自由カメラ移動時は cameraObject_ (プレイヤーの親) を同期しないことで、
            // プレイヤーは元の位置に取り残される（=自由に見回せる）ようにする
        }

        // --- ルート固定移動（レールに沿った移動） ---
        if (isPlaying_)
        {
            // === ブースト機能の処理 ===
            auto pp = EngineServices::GetInstance()->GetPostProcess();
            if (player_ && player_->IsBoosting())
            {
                gameSpeed_ = 1.5f; // スピードアップ
                thrusterEffect_.SetBaseColor({ 1.0f, 0.2f, 0.0f, 1.0f }); // 赤色
                if (pp)
                {
                    pp->SetEffectActive("RadialBlur", true);
                    pp->GetData()->radialBlurIntensity = 0.1f;
                }
            }
            else
            {
                gameSpeed_ = 1.0f; // 通常スピード
                thrusterEffect_.SetBaseColor({ 1.0f, 1.0f, 1.0f, 1.0f }); // 通常（元の色）
                if (pp)
                {
                    pp->SetEffectActive("RadialBlur", false);
                }
            }
            if (!mainRails_.empty() && railCameraController_)
            {
                railCameraController_->Update(gameSpeed_, player_->GetTranslate());
            }
            else
            {
                // レールが無い場合のフォールバック（自動前進）
                const float kAutoSpeed = 0.05f;
                Vector3 camPos = activeCamera_->GetTranslate();
                camPos.z += kAutoSpeed;
                activeCamera_->SetTranslate(camPos);

                if (cameraObject_)
                {
                    cameraObject_->SetTranslate(camPos);
                    cameraObject_->Update();
                }
            }
        }
    }

    // プレイヤーの更新
    if (player_)
    {
        if (isPlaying_)
        {
            player_->Update(bullets_, cameraObject_.get());

            // 回避エフェクトの再生
            if (player_->ConsumeDodgeTrigger())
            {
                const Matrix4x4& wMat = player_->GetObject3d()->GetmatWorld();
                // ワールド座標を取得
                Vector3 pPos = { wMat.m[3][0], wMat.m[3][1], wMat.m[3][2] };

                // 進行方向（Z軸）
                Vector3 forward = { wMat.m[2][0], wMat.m[2][1], wMat.m[2][2] };
                float len = std::sqrt(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
                if (len > 0.0001f)
                {
                    forward.x /= len; forward.y /= len; forward.z /= len;
                }

                // 自機より少し手前（進行方向の逆）に出す
                Vector3 effectPos = { pPos.x - forward.x * 2.0f, pPos.y - forward.y * 2.0f, pPos.z - forward.z * 2.0f };

                dodgeEffect_.SetPosition(effectPos);
                dodgeEffect_.Play();
            }
        }
        else
        {
            // ゲーム停止中でも、Object3dの更新(カメラ行列の反映など)は必要
            player_->Update3DObjectOnly();
        }
    }

    // 弾の更新
    if (isPlaying_)
    {
        for (auto it = bullets_.begin(); it != bullets_.end(); )
        {
            (*it)->Update();
            if ((*it)->IsDead())
            {
                it = bullets_.erase(it);
            }
            else
            {
                ++it;
            }
        }

        Vector3 cameraPos = activeCamera_->GetTranslate();
        Vector3 rot = activeCamera_->GetRotation();
        Vector3 cameraForward = { std::sinf(rot.y), 0.0f, std::cosf(rot.y) };

        // 敵の更新と当たり判定
        for (auto it = enemies_.begin(); it != enemies_.end();)
        {
            (*it)->Update(cameraPos, cameraForward, player_.get(), enemyBullets_);

            // プレイヤーの弾との当たり判定
            for (auto& bullet : bullets_)
            {
                if (bullet->IsDead()) continue;

                Sphere bulletSphere = { bullet->GetPosition(), 1.0f };
                bool isHit = false;

                isHit = (*it)->CheckCollision(bulletSphere);

                // CCD (Continuous Collision Detection): すり抜け防止
                if (!isHit)
                {
                    Vector3 prev = bullet->GetPreviousPosition();
                    Vector3 curr = bullet->GetPosition();
                    Vector3 diff = { curr.x - prev.x, curr.y - prev.y, curr.z - prev.z };
                    float moveLen = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
                    if (moveLen > 0.0001f)
                    {
                        Ray moveRay = { prev, { diff.x / moveLen, diff.y / moveLen, diff.z / moveLen } };
                        float hitDist = 0.0f;
                        if ((*it)->CheckRaycast(moveRay, &hitDist))
                        {
                            if (hitDist <= moveLen + 1.0f)
                            { // 弾の半径分だけ余裕を持たせる
                                isHit = true;
                            }
                        }
                    }
                }

                if (isHit)
                {
                    bullet->OnCollision();
                    (*it)->OnCollision();

                    // パーティクルの再生
                    hitEffect_.SetPosition(bulletSphere.center);
                    hitEffect_.Play();
                }
            }

            if ((*it)->IsDead())
            {
                // 破壊エフェクト
                explosionEffect_.SetPosition((*it)->GetPosition());
                explosionEffect_.Play();
                it = enemies_.erase(it);
            }
            else
            {
                ++it;
            }
        }

        // 敵弾の更新とプレイヤーとの当たり判定
        for (auto it = enemyBullets_.begin(); it != enemyBullets_.end();)
        {
            (*it)->Update();

            if (!(*it)->IsDead() && player_ && !player_->IsDead())
            {
                Sphere bulletSphere = { (*it)->GetPosition(), 1.0f }; // 弾の当たり判定サイズは適宜調整

                // プレイヤー側の当たり判定（OBB）
                const Matrix4x4& wMat = player_->GetColliderObject()->GetmatWorld();
                Vector3 pWorldPos = { wMat.m[3][0], wMat.m[3][1], wMat.m[3][2] };
                Vector3 playerBoxSize = player_->GetColliderSize();

                // 回転行列の抽出と正規化
                Matrix4x4 rotMat = wMat;
                for (int i = 0; i < 3; ++i)
                {
                    float len = std::sqrt(rotMat.m[i][0] * rotMat.m[i][0] + rotMat.m[i][1] * rotMat.m[i][1] + rotMat.m[i][2] * rotMat.m[i][2]);
                    if (len > 0.0001f)
                    {
                        rotMat.m[i][0] /= len;
                        rotMat.m[i][1] /= len;
                        rotMat.m[i][2] /= len;
                    }
                }

                OBB playerOBB = CollisionMath::CreateOBB(pWorldPos, playerBoxSize, rotMat);

                if (CollisionMath::IsCollision(bulletSphere, playerOBB))
                {
                    (*it)->OnCollision();
                    hitEffect_.SetPosition((*it)->GetPosition());
                    hitEffect_.Play();

                    player_->OnCollision();
                }
            }

            if ((*it)->IsDead())
            {
                it = enemyBullets_.erase(it);
            }
            else
            {
                ++it;
            }
        }

        // 障害物の更新と当たり判定
        for (auto it = obstacles_.begin(); it != obstacles_.end();)
        {
            (*it)->Update();

            // プレイヤーの弾との当たり判定
            for (auto& bullet : bullets_)
            {
                if (bullet->IsDead()) continue;

                Sphere bulletSphere = { bullet->GetPosition(), 1.0f };
                bool isHit = false;

                isHit = (*it)->CheckCollision(bulletSphere);

                // CCD (Continuous Collision Detection): すり抜け防止
                if (!isHit)
                {
                    Vector3 prev = bullet->GetPreviousPosition();
                    Vector3 curr = bullet->GetPosition();
                    Vector3 diff = { curr.x - prev.x, curr.y - prev.y, curr.z - prev.z };
                    float moveLen = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
                    if (moveLen > 0.0001f)
                    {
                        Ray moveRay = { prev, { diff.x / moveLen, diff.y / moveLen, diff.z / moveLen } };
                        float hitDist = 0.0f;
                        if ((*it)->CheckRaycast(moveRay, &hitDist))
                        {
                            if (hitDist <= moveLen + 1.0f)
                            { // 弾の半径分だけ余裕を持たせる
                                isHit = true;
                            }
                        }
                    }
                }

                if (isHit)
                {
                    bullet->OnCollision();
                    (*it)->OnCollision();

                    // パーティクルの再生
                    hitEffect_.SetPosition(bulletSphere.center);
                    hitEffect_.Play();
                }
            }

            if ((*it)->IsDead())
            {
                // 破壊エフェクト
                explosionEffect_.SetPosition((*it)->GetPosition());
                explosionEffect_.Play();
                it = obstacles_.erase(it);
            }
            else
            {
                ++it;
            }
        }

        // --- 照準（レティクル）のロックオン判定 (Raycast) ---
        bool isLockOn = false;
        if (player_ && player_->GetObject3d() && player_->GetReticle() && activeCamera_)
        {
            Vector3 cameraPos = activeCamera_->GetTranslate();
            Vector3 reticlePos = player_->GetReticleWorldPosition();

            // カメラから照準へ向かうレイを作成（画面上で重なっているかを判定）
            Vector3 rayDir = {
                reticlePos.x - cameraPos.x,
                reticlePos.y - cameraPos.y,
                reticlePos.z - cameraPos.z
            };
            float length = std::sqrtf(rayDir.x * rayDir.x + rayDir.y * rayDir.y + rayDir.z * rayDir.z);

            Vector3 lockOnPos = { 0, 0, 0 };
            Enemy* lockOnEnemy = nullptr;

            if (length > 0.0001f)
            {
                rayDir.x /= length;
                rayDir.y /= length;
                rayDir.z /= length;

                Ray ray = { cameraPos, rayDir };

                float bestScore = 1000000.0f;

                // 全ての敵に対してレイキャストを行う
                for (auto& enemy : enemies_)
                {
                    if (enemy->IsDead()) continue;

                    float dist = 0.0f;
                    bool hit = enemy->CheckRaycast(ray, &dist);

                    if (hit)
                    {
                        // 敵の中心位置を取得してレイ方向との角度を計算
                        Vector3 enemyPos = enemy->GetPosition();
                        Vector3 toEnemy = { enemyPos.x - cameraPos.x, enemyPos.y - cameraPos.y, enemyPos.z - cameraPos.z };
                        float toEnemyLen = std::sqrt(toEnemy.x * toEnemy.x + toEnemy.y * toEnemy.y + toEnemy.z * toEnemy.z);
                        if (toEnemyLen > 0.0f)
                        {
                            toEnemy.x /= toEnemyLen;
                            toEnemy.y /= toEnemyLen;
                            toEnemy.z /= toEnemyLen;
                        }

                        // 内積から角度（ラジアン）を求める
                        float dot = ray.direction.x * toEnemy.x + ray.direction.y * toEnemy.y + ray.direction.z * toEnemy.z;
                        float angle = std::acos(std::clamp(dot, -1.0f, 1.0f));

                        // 角度（照準の中心にどれだけ近いか）を最優先し、距離も少し考慮するスコア
                        float score = angle * 100.0f + dist * 0.1f;

                        if (score < bestScore)
                        {
                            bestScore = score;
                            lockOnPos = {
                                ray.origin.x + ray.direction.x * dist,
                                ray.origin.y + ray.direction.y * dist,
                                ray.origin.z + ray.direction.z * dist
                            };
                            isLockOn = true;
                            lockOnEnemy = enemy.get();
                        }
                    }
                }
            }

            // ロックオンはしていないが、近い敵がいるかどうかの判定
            float minEnemyDist = 1000000.0f;
            for (auto& enemy : enemies_)
            {
                if (enemy->IsDead()) continue;
                Vector3 ePos = enemy->GetPosition();
                float d = std::sqrt((ePos.x - cameraPos.x) * (ePos.x - cameraPos.x) +
                    (ePos.y - cameraPos.y) * (ePos.y - cameraPos.y) +
                    (ePos.z - cameraPos.z) * (ePos.z - cameraPos.z));
                if (d < minEnemyDist)
                {
                    minEnemyDist = d;
                }
            }

            // 照準の色の更新とロックオン座標の伝達
            if (isLockOn)
            {
                player_->SetReticleColor({ 1.0f, 0.0f, 0.0f, 1.0f }); // 赤色
                player_->SetLockOn(true, lockOnPos, lockOnEnemy);
            }
            else
            {
                if (minEnemyDist < 100.0f)
                {
                    // 近くに敵がいる場合はオレンジ色（警戒）
                    player_->SetReticleColor({ 1.0f, 0.6f, 0.0f, 1.0f });
                }
                else
                {
                    player_->SetReticleColor({ 1.0f, 1.0f, 1.0f, 1.0f }); // 白色
                }
                player_->SetLockOn(false);
            }
        }

        // 当たり判定で死んだ弾を削除
        for (auto it = bullets_.begin(); it != bullets_.end(); )
        {
            if ((*it)->IsDead())
            {
                it = bullets_.erase(it);
            }
            else
            {
                ++it;
            }
        }

    }
    else
    {
        for (auto& bullet : bullets_)
        {
            bullet->Update3DObjectOnly();
        }
        for (auto& enemy : enemies_)
        {
            enemy->Update3DObjectOnly();
        }
    }

    // ESC 押下でウィンドウを閉じる
    if (input && input->TriggerKey(DIK_ESCAPE))
    {
        auto dxCommon = services->GetDirectXCommon();
        if (dxCommon)
        {
            WinApp* winApp = dxCommon->GetWinApp();
            if (winApp)
            {
                ::PostMessage(winApp->GetHwnd(), WM_CLOSE, 0, 0);
            }
        }
    }

    // カメラ更新（入力反映後に行う）
    if (activeCamera_) activeCamera_->Update();
    for (auto& model : modelInstances) if (model) model->Update();
    if (isDrawRail_)
    {
        for (auto& vis : railVisualizers_) if (vis) vis->Update();
        for (auto& vis : enemyRailVisualizers_) if (vis) vis->Update();
    }

    // ポーズ中でもカメラが動いた場合に行列を更新する
    if (!isPlaying_)
    {
        for (auto& enemy : enemies_) if (enemy) enemy->Update3DObjectOnly();
        for (auto& obstacle : obstacles_) if (obstacle) obstacle->Update3DObjectOnly();
    }

    // カメラ行列の取得
    Matrix4x4 cameraMatrix = Matrix4x4::Identity();
    Matrix4x4 viewMatrix = Matrix4x4::Identity();
    Matrix4x4 projectionMatrix = Matrix4x4::Identity();
    Matrix4x4 billboardMatrix = Matrix4x4::Identity();

    if (activeCamera_)
    {
        cameraMatrix = activeCamera_->GetWorldMatrix();
        viewMatrix = activeCamera_->GetViewMatrix();
        projectionMatrix = activeCamera_->GetProjectionMatrix();
        billboardMatrix = Billboard::CreateFromCamera(activeCamera_, true);
    }

    if (skybox_)
    {
        skybox_->SetCamera(activeCamera_);
        skybox_->Update();
    }

    if (isPlaying_)
    {
        // プレイ中のみ特定の更新を行う場合はここに記述
    }

    // エフェクトの更新はプレイ中・停止中（エディタ操作中）に関わらず常に実行する
    if (player_ && player_->GetObject3d())
    {
        const Matrix4x4& wMat = player_->GetObject3d()->GetmatWorld();
        Vector3 worldPos = { wMat.m[3][0], wMat.m[3][1], wMat.m[3][2] };

        // 自機の後方（ローカルZ軸の逆方向）へオフセットをかける
        Vector3 backward = { -wMat.m[2][0], -wMat.m[2][1], -wMat.m[2][2] };
        float length = std::sqrt(backward.x * backward.x + backward.y * backward.y + backward.z * backward.z);
        if (length > 0.0f)
        {
            backward.x /= length; backward.y /= length; backward.z /= length;
        }
        float offsetDistance = 1.8f; // 尻尾までの距離（必要に応じて調整）
        worldPos.x += backward.x * offsetDistance;
        worldPos.y += backward.y * offsetDistance;
        worldPos.z += backward.z * offsetDistance;

        thrusterEffect_.SetPosition(worldPos); // スラスターは常に自機の尻尾に追従
        thrusterEffect_.Update(dt, viewMatrix, projectionMatrix, billboardMatrix);
        explosionEffect_.Update(dt, viewMatrix, projectionMatrix, billboardMatrix);
        hitEffect_.Update(dt, viewMatrix, projectionMatrix, billboardMatrix);
        dodgeEffect_.Update(dt, viewMatrix, projectionMatrix, billboardMatrix);
    }

#ifdef USE_IMGUI

    ImGui::Begin("Game Control");

    bool doReset = false;

    // シーン切り替えとプレイ状態
    if (isPlaying_)
    {
        if (ImGui::Button("Stop (Pause)", ImVec2(120, 40)))
        {
            isPlaying_ = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset", ImVec2(120, 40)))
        {
            doReset = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Full Screen (Hide UI)", ImVec2(160, 40)))
        {
            EngineServices::GetInstance()->SetEditorMode(false);
        }
        ImGui::Text("Status: PLAYING");
    }
    else
    {
        if (ImGui::Button("Play (Start)", ImVec2(120, 40)))
        {
            isPlaying_ = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Play Full Screen", ImVec2(160, 40)))
        {
            isPlaying_ = true;
            EngineServices::GetInstance()->SetEditorMode(false);
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset", ImVec2(120, 40)))
        {
            doReset = true;
        }
        ImGui::Text("Status: STOPPED (Free Camera Mode)");
        ImGui::Text("Camera Control: WASD/QE to move, Right-Click Drag to rotate");
    }

    ImGui::Separator();
    if (ImGui::Button("Reload Level (F5)", ImVec2(160, 40)))
    {
        ReloadLevel();
    }
    ImGui::SameLine();
    if (ImGui::Button("Respawn Enemies", ImVec2(160, 40)))
    {
        ReloadEnemiesOnly();
    }

    ImGui::Separator();
    if (railCameraController_)
    {
        float p = railCameraController_->GetProgress();
        if (ImGui::SliderFloat("ゲーム時間 (Rail Progress)", &p, 0.0f, 1.0f))
        {
            railCameraController_->SetProgress(p);
        }
    }
    ImGui::SliderFloat("ゲームスピード (Game Speed)", &gameSpeed_, 0.0f, 5.0f);

    // リセット処理：レール進行度を0に戻し、カメラとプレイヤーを始点に移動させる
    if (doReset)
    {
        isPlaying_ = false;
        if (railCameraController_)
        {
            railCameraController_->Reset();
        }

        // リセット時に補間用変数を初期化
        if (!mainRails_.empty() && mainRails_[0]->IsValid())
        {
            Vector3 railForward = mainRails_[0]->GetForward(0.0f);
            float yaw = std::atan2(railForward.x, railForward.z);
            float pitch = std::asin(-railForward.y);
            float railTilt = mainRails_[0]->GetTilt(0.0f);
            currentCameraRot_ = { pitch, yaw, 0.0f };
            lastCameraYaw_ = yaw;
            currentCameraBank_ = railTilt;
        }
    }

    ImGui::Separator();
    ImGui::Checkbox("レールを表示 (Draw Rail)", &isDrawRail_);
    ImGui::Checkbox("コライダーを表示 (Draw Collider)", &isDrawCollider_);
    ImGui::End();

    if (player_)
    {
        player_->DrawUI();
    }

    // --- Sprite ウィンドウ ---
    ImGui::Begin("スプライト");
    if (!sprites.empty())
    {
        // unique_ptr から生ポインタを取得
        Sprite* s = sprites[0].get();

        bool display = isDisplaySprite;
        if (ImGui::Checkbox("スプライト表示 (Display Sprite)", &display))
        {
            isDisplaySprite = display;
        }

        Vector2 pos = s->GetPosition();
        float posArr[2] = { pos.x, pos.y };
        if (ImGui::DragFloat2("座標 (Position)", posArr, 1.0f))
        {
            s->SetPosition(Vector2(posArr[0], posArr[1]));
        }

        Vector2 size = s->GetSize();
        float sizeArr[2] = { size.x, size.y };
        if (ImGui::DragFloat2("サイズ (Size)", sizeArr, 1.0f, 1.0f, 4096.0f))
        {
            s->SetSize(Vector2(sizeArr[0], sizeArr[1]));
        }

        float rotation = s->GetRotation();
        if (ImGui::DragFloat("回転 (Rotation)", &rotation, 0.5f))
        {
            s->SetRotation(rotation);
        }

        Vector4 col = s->GetColor();
        float colArr[4] = { col.x, col.y, col.z, col.w };
        if (ImGui::ColorEdit4("色 (Color)", colArr))
        {
            s->SetColor(Vector4(colArr[0], colArr[1], colArr[2], colArr[3]));
        }
    }
    ImGui::End();


    std::vector<Object3d*> allModels;
    std::vector<std::string> modelNames;

    int index = 0;
    for (auto& obj : modelInstances)
    {
        if (obj)
        {
            allModels.push_back(obj.get());
            modelNames.push_back("Model " + std::to_string(index));
        }
        index++;
    }
    if (player_)
    {
        if (player_->GetObject3d())
        {
            allModels.push_back(player_->GetObject3d());
            modelNames.push_back("Player");
        }
        if (player_->GetReticle())
        {
            allModels.push_back(player_->GetReticle());
            modelNames.push_back("Player Reticle");
        }
    }

    // --- Model ウィンドウ ---
    ImGui::Begin("モデル");
    if (!allModels.empty())
    {
        static int currentModelIndex = 0;
        if (currentModelIndex >= allModels.size()) currentModelIndex = 0;

        std::vector<const char*> namePtrs;
        for (const auto& name : modelNames)
        {
            namePtrs.push_back(name.c_str());
        }

        ImGui::Combo("対象モデル", &currentModelIndex, namePtrs.data(), (int)namePtrs.size());

        Object3d* obj = allModels[currentModelIndex];

        // Translate / Rotation / Scale
        Vector3 t = obj->GetTranslate();
        float tArr[3] = { t.x, t.y, t.z };
        if (ImGui::DragFloat3("座標 (Translate)", tArr, 0.05f))
        {
            obj->SetTranslate(Vector3(tArr[0], tArr[1], tArr[2]));
        }

        Vector3 r = obj->GetRotation();
        float rArr[3] = { r.x, r.y, r.z };
        if (ImGui::DragFloat3("回転 (Rotation)", rArr, 0.5f))
        {
            obj->SetRotation(Vector3(rArr[0], rArr[1], rArr[2]));
        }

        Vector3 s = obj->GetScale();
        float sArr[3] = { s.x, s.y, s.z };
        if (ImGui::DragFloat3("スケール (Scale)", sArr, 0.01f, 0.001f, 100.0f))
        {
            obj->SetScale(Vector3(sArr[0], sArr[1], sArr[2]));
        }

        ImGui::Separator();

        // 反射強度のスライダーを追加
        if (obj->GetModel())
        {
            float envCoeff = obj->GetModel()->GetEnvironmentCoefficient();
            if (ImGui::DragFloat("環境反射係数 (Environment Coeff)", &envCoeff, 0.01f, 0.0f, 1.0f))
            {
                obj->SetEnvironmentCoefficient(envCoeff);
            }
        }

        ImGui::Separator();

        // 最初のインスタンスの Model を参照して現在値を取得
        Model* sampleModel = allModels[0]->GetModel();
        if (sampleModel)
        {
            int currentSelect = sampleModel->GetSelectLightings();

            // ラベルは HLSL の case に対応させる（0..5）
            const char* lightingNames[] = {
                "0: テクスチャのみ (TextureOnly)",
                "1: 平行光源・ディフューズ (Directional Diffuse)",
                "2: 平行光源・ソフト (Directional Soft)",
                "3: 平行光源・スペキュラ (Dir Diffuse+Specular)",
                "4: 平行光源 + 点光源 (Dir + Point)",
                "5: スポットライト (Spot)"
            };

            // Combo で選択（HLSL の switch の case に対応）
            if (ImGui::Combo("ライティングモード (一括変更)", &currentSelect, lightingNames, IM_ARRAYSIZE(lightingNames)))
            {
                // 全インスタンスに反映
                for (auto& mObj : allModels)
                {
                    Model* m = mObj->GetModel();
                    if (m) m->SetSelectLightings(currentSelect);
                }
            }
        }
    }
    ImGui::End();



    // --- Camera ウィンドウ (分離) ---
    ImGui::Begin("カメラ");
    if (camera)
    {
        Vector3& camPosRef = camera->GetTranslate();
        float camPosArr[3] = { camPosRef.x, camPosRef.y, camPosRef.z };
        if (ImGui::DragFloat3("座標 (Translate)", camPosArr, 0.1f))
        {
            camera->SetTranslate(Vector3(camPosArr[0], camPosArr[1], camPosArr[2]));
        }

        Vector3& camRotRef = camera->GetRotation();
        float camRotArr[3] = { camRotRef.x, camRotRef.y, camRotRef.z };
        if (ImGui::DragFloat3("回転 (Rotation)", camRotArr, 0.1f))
        {
            camera->SetRotation(Vector3(camRotArr[0], camRotArr[1], camRotArr[2]));
        }

        float fov = camera->GetFovY();
        if (ImGui::DragFloat("画角 (FOV Y)", &fov, 0.01f, 0.01f, 3.14f))
        {
            camera->SetFovY(fov);
        }

        float aspect = camera->GetAspectRatio();
        if (ImGui::DragFloat("アスペクト比", &aspect, 0.01f, 0.1f, 10.0f))
        {
            camera->SetAspectRatio(aspect);
        }

        float nearC = camera->GetNearClip();
        if (ImGui::DragFloat("近クリップ", &nearC, 0.001f, 0.001f, 100.0f))
        {
            camera->SetNearClip(nearC);
        }

        float farC = camera->GetFarClip();
        if (ImGui::DragFloat("遠クリップ", &farC, 0.1f, 1.0f, 1000.0f))
        {
            camera->SetFarClip(farC);
        }
    }
    ImGui::End();


    // --- Light ウィンドウ (モデルが使っているライトのみ表示) ---
    ImGui::Begin("グローバルライト設定");
    if (!allModels.empty())
    {
        Object3d* firstObj = allModels[0];
        Model* sampleModel = firstObj ? firstObj->GetModel() : nullptr;
        int lightingMode = sampleModel ? sampleModel->GetSelectLightings() : 0;

        // ヘルパー: ライティングモードがどのライトを使うか
        auto usesDirectional = [](int mode)
            {
                return mode == 1 || mode == 2 || mode == 3 || mode == 4;
            };
        auto usesPoint = [](int mode)
            {
                return mode == 4;
            };
        auto usesSpot = [](int mode)
            {
                return mode == 5;
            };

        // Directional
        if (usesDirectional(lightingMode))
        {
            ImGui::Separator();
            ImGui::Text("平行光源 (Directional Light)");

            static bool dirInit = false;
            static Vector4 dirColor = { 1.0f, 1.0f, 1.0f, 1.0f };
            static Vector3 dirDirection = { 0.0f, -1.0f, 1.0f }; // 正面斜め上から当たるように初期値を変更
            static float dirIntensity = 1.0f;
            static bool dirEnabledGlobal = true;
            static float dirPrevIntensityGlobal = 1.0f;

            if (!dirInit)
            {
                dirColor = firstObj->GetDirectionalLightColor();
                dirDirection = firstObj->GetDirectionalLightDirection();
                dirIntensity = firstObj->GetDirectionalLightIntensity();
                dirPrevIntensityGlobal = dirIntensity;
                dirInit = true;
            }

            float dcArr[4] = { dirColor.x, dirColor.y, dirColor.z, dirColor.w };
            if (ImGui::ColorEdit4("色 (Color)##Dir", dcArr))
            {
                dirColor = Vector4(dcArr[0], dcArr[1], dcArr[2], dcArr[3]);
                for (auto& m : allModels) m->SetDirectionalLightColor(dirColor);
            }

            float ddArr[3] = { dirDirection.x, dirDirection.y, dirDirection.z };
            if (ImGui::DragFloat3("方向 (Direction)##Dir", ddArr, 0.01f, -10.0f, 10.0f))
            {
                dirDirection = Vector3(ddArr[0], ddArr[1], ddArr[2]);
                for (auto& m : allModels) m->SetDirectionalLightDirection(dirDirection);
            }

            if (ImGui::DragFloat("強度 (Intensity)##Dir", &dirIntensity, 0.01f, 0.0f, 100.0f))
            {
                if (dirEnabledGlobal)
                {
                    for (auto& m : allModels) m->SetDirectionalLightIntensity(dirIntensity);
                    dirPrevIntensityGlobal = dirIntensity;
                }
                else
                {
                    dirPrevIntensityGlobal = dirIntensity;
                }
            }

            if (ImGui::Checkbox("平行光源を有効化 (global)##Dir", &dirEnabledGlobal))
            {
                if (!dirEnabledGlobal)
                {
                    for (auto& m : allModels) m->SetDirectionalLightIntensity(0.0f);
                }
                else
                {
                    for (auto& m : allModels) m->SetDirectionalLightIntensity(dirPrevIntensityGlobal);
                }
            }
        }

        // Point
        if (usesPoint(lightingMode))
        {
            ImGui::Separator();
            ImGui::Text("点光源 (Point Light)");

            static bool pointInit = false;
            static Vector4 pointColor = { 1.0f, 1.0f, 1.0f, 1.0f };
            static Vector3 pointPosition = { 0.0f, 1.0f, -8.0f }; // スザンヌの正面に初期配置
            static float pointIntensity = 1.0f;
            static float prevPointIntensity = 1.0f;
            static float pointRadius = 15.0f; // 初期値1.0では光が届かないため15.0に拡大
            static float pointRange = 1.0f;
            static bool pointLightEnabled = true;

            if (!pointInit && firstObj)
            {
                pointColor = firstObj->GetPointLightColor();
                pointPosition = firstObj->GetPointLightPosition();
                pointIntensity = firstObj->GetPointLightIntensity();
                prevPointIntensity = pointIntensity;
                pointInit = true;
            }

            if (ImGui::Checkbox("点光源を有効化##Point", &pointLightEnabled))
            {
                if (!pointLightEnabled)
                {
                    for (auto& m : allModels) m->SetPointLightIntensity(0.0f);
                }
                else
                {
                    for (auto& m : allModels) m->SetPointLightIntensity(prevPointIntensity);
                }
            }

#if defined(IMGUI_VERSION) && (IMGUI_VERSION_NUM >= 18000)
            ImGui::BeginDisabled(!pointLightEnabled);
#else
            if (!pointLightEnabled)
            {
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            }
#endif

            float pcArr[4] = { pointColor.x, pointColor.y, pointColor.z, pointColor.w };
            if (ImGui::ColorEdit4("色 (Color)##Point", pcArr))
            {
                pointColor = Vector4(pcArr[0], pcArr[1], pcArr[2], pcArr[3]);
                for (auto& m : allModels) m->SetPointLightColor(pointColor);
            }

            float ppArr[3] = { pointPosition.x, pointPosition.y, pointPosition.z };
            if (ImGui::DragFloat3("位置 (Position)##Point", ppArr, 0.05f, -100.0f, 100.0f))
            {
                pointPosition = Vector3(ppArr[0], ppArr[1], ppArr[2]);
                for (auto& m : allModels) m->SetPointLightPosition(pointPosition);
            }

            if (ImGui::DragFloat("強度 (Intensity)##Point", &pointIntensity, 0.01f, 0.0f, 100.0f))
            {
                if (pointLightEnabled)
                {
                    for (auto& m : allModels) m->SetPointLightIntensity(pointIntensity);
                    prevPointIntensity = pointIntensity;
                }
                else
                {
                    prevPointIntensity = pointIntensity;
                }
            }

            if (ImGui::DragFloat("半径 (Radius)##Point", &pointRadius, 0.01f, 0.1f, 100.0f))
            {
                for (auto& m : allModels) m->SetPointLightRadius(pointRadius);
            }
            if (ImGui::DragFloat("減衰範囲 (Decay Range)##Point", &pointRange, 0.01f, 0.1f, 50.0f))
            {
                for (auto& m : allModels) m->SetPointLightDecry(pointRange);
            }

#if defined(IMGUI_VERSION) && (IMGUI_VERSION_NUM >= 18000)
            ImGui::EndDisabled();
#else
            if (!pointLightEnabled)
            {
                ImGui::PopItemFlag();
                ImGui::PopStyleVar();
            }
#endif
        }

        // Spot
        if (usesSpot(lightingMode))
        {
            ImGui::Separator();
            ImGui::Text("スポットライト (Spot Light)");

            static bool spotInit = false;
            static Vector4 spotColor = { 1.0f, 1.0f, 1.0f, 1.0f };
            static Vector3 spotPosition = { 0.0f, 5.0f, 0.0f };
            static Vector3 spotDirection = { 0.0f, -1.0f, 0.0f };
            static float spotIntensity = 1.0f;
            static float prevSpotIntensity = 1.0f;
            static float spotDistance = 10.0f;
            static float spotDecay = 1.0f;
            static float spotAngleDeg = 45.0f;
            static bool spotLightEnabled = true;

            if (!spotInit && firstObj)
            {
                spotColor = firstObj->GetSpotLightColor();
                spotPosition = firstObj->GetSpotLightPosition();
                spotDirection = firstObj->GetSpotLightDirection();
                spotIntensity = firstObj->GetSpotLightIntensity();
                prevSpotIntensity = spotIntensity;
                spotDistance = firstObj->GetSpotLightDistance();
                spotDecay = firstObj->GetSpotLightDecay();
                spotAngleDeg = firstObj->GetSpotLightAngleDeg();
                spotInit = true;
            }

            if (ImGui::Checkbox("スポットライトを有効化##Spot", &spotLightEnabled))
            {
                if (!spotLightEnabled)
                {
                    for (auto& m : allModels) m->SetSpotLightIntensity(0.0f);
                }
                else
                {
                    for (auto& m : allModels) m->SetSpotLightIntensity(prevSpotIntensity);
                }
            }

#if defined(IMGUI_VERSION) && (IMGUI_VERSION_NUM >= 18000)
            ImGui::BeginDisabled(!spotLightEnabled);
#else
            if (!spotLightEnabled)
            {
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            }
#endif

            // Color
            {
                float scArr[4] = { spotColor.x, spotColor.y, spotColor.z, spotColor.w };
                if (ImGui::ColorEdit4("色 (Color)##Spot", scArr))
                {
                    spotColor = Vector4(scArr[0], scArr[1], scArr[2], scArr[3]);
                    for (auto& m : allModels) m->SetSpotLightColor(spotColor);
                }
            }

            // Position
            {
                float spArr[3] = { spotPosition.x, spotPosition.y, spotPosition.z };
                if (ImGui::DragFloat3("位置 (Position)##Spot", spArr, 0.05f, -100.0f, 100.0f))
                {
                    spotPosition = Vector3(spArr[0], spArr[1], spArr[2]);
                    for (auto& m : allModels) m->SetSpotLightPosition(spotPosition);
                }
            }

            // Direction
            {
                float sdArr[3] = { spotDirection.x, spotDirection.y, spotDirection.z };
                if (ImGui::DragFloat3("方向 (Direction)##Spot", sdArr, 0.01f, -10.0f, 10.0f))
                {
                    spotDirection = Vector3(sdArr[0], sdArr[1], sdArr[2]);
                    for (auto& m : allModels) m->SetSpotLightDirection(spotDirection);
                }
            }

            // Intensity
            if (ImGui::DragFloat("強度 (Intensity)##Spot", &spotIntensity, 0.01f, 0.0f, 100.0f))
            {
                if (spotLightEnabled)
                {
                    for (auto& m : allModels) m->SetSpotLightIntensity(spotIntensity);
                    prevSpotIntensity = spotIntensity;
                }
                else
                {
                    prevSpotIntensity = spotIntensity;
                }
            }

            // Distance / Decay
            if (ImGui::DragFloat("距離 (Distance)##Spot", &spotDistance, 0.1f, 0.0f, 10000.0f))
            {
                for (auto& m : allModels) m->SetSpotLightDistance(spotDistance);
            }
            if (ImGui::DragFloat("減衰率 (Decay)##Spot", &spotDecay, 0.01f, 0.0f, 10.0f))
            {
                for (auto& m : allModels) m->SetSpotLightDecay(spotDecay);
            }

            // Angle (deg)
            if (ImGui::SliderFloat("角度 (Angle deg)##Spot", &spotAngleDeg, 1.0f, 90.0f))
            {
                for (auto& m : allModels) m->SetSpotLightAngleDeg(spotAngleDeg);
            }

#if defined(IMGUI_VERSION) && (IMGUI_VERSION_NUM >= 18000)
            ImGui::EndDisabled();
#else
            if (!spotLightEnabled)
            {
                ImGui::PopItemFlag();
                ImGui::PopStyleVar();
            }
#endif
        }

        // モデルがライトを使わない場合は何も表示されない（意図的）
        if (!usesDirectional(lightingMode) && !usesPoint(lightingMode) && !usesSpot(lightingMode))
        {
            ImGui::TextWrapped("このモデルのライティングモードでは、編集可能なライトがありません。");
        }
    }
    ImGui::End();


    // --- Particle ウィンドウ ---
    ImGui::Begin("Effect Selector");
    const char* items[] = { "Thruster", "Explosion", "Hit" };
    ImGui::Combo("Edit Target", &currentEditEffectIndex_, items, IM_ARRAYSIZE(items));
    ImGui::End();

    if (currentEditEffectIndex_ == 0) thrusterEffect_.DrawImGui();
    else if (currentEditEffectIndex_ == 1) explosionEffect_.DrawImGui();
    else if (currentEditEffectIndex_ == 2) hitEffect_.DrawImGui();


#endif // USE_IMGUI

}

void GamePlayScene::Draw()
{
    auto services = EngineServices::GetInstance();
    auto object3dCommon = services->GetObject3dCommon();
    auto spriteCommon = services->GetSpriteCommon();

    if (skybox_)
    {
        skybox_->Draw();
    }

    if (spriteCommon) spriteCommon->SetCommonDrawSetting();

    if (isDisplaySprite)
    {
        for (auto& sprite : sprites) if (sprite) sprite->Draw();
    }

    if (object3dCommon) object3dCommon->SetCommonDrawSetting();

    for (auto& model : modelInstances) if (model) model->Draw();


    // 敵とコライダーの描画
    if (object3dCommon) object3dCommon->SetCommonDrawSetting();
    for (auto& enemy : enemies_)
    {
        enemy->Draw();
    }
    for (auto& obstacle : obstacles_)
    {
        obstacle->Draw();
    }

    // コライダーはワイヤーフレームで描画
    if (isDrawCollider_)
    {
        if (object3dCommon) object3dCommon->SetWireframeDrawSetting();
        for (auto& enemy : enemies_)
        {
            enemy->DrawCollider();
        }
        for (auto& obstacle : obstacles_)
        {
            obstacle->DrawCollider();
        }
        for (auto& bullet : bullets_)
        {
            bullet->DrawCollider();
        }
        for (auto& bullet : enemyBullets_)
        {
            bullet->DrawCollider();
        }
        if (player_)
        {
            player_->DrawCollider();
        }
        // 描画設定を元に戻す
        if (object3dCommon) object3dCommon->SetCommonDrawSetting();
    }

    // 弾の描画
    for (auto& bullet : bullets_)
    {
        bullet->Draw();
    }
    for (auto& bullet : enemyBullets_)
    {
        bullet->Draw();
    }

    // プレイヤーの描画
    if (player_)
    {
        if (object3dCommon) object3dCommon->SetCommonDrawSetting();
        player_->Draw();
    }

    // レールの描画
    if (isDrawRail_)
    {
        if (object3dCommon) object3dCommon->SetCommonDrawSetting();
        for (auto& vis : railVisualizers_)
        {
            if (vis) vis->Draw();
        }
        for (auto& vis : enemyRailVisualizers_)
        {
            if (vis) vis->Draw();
        }
    }

    thrusterEffect_.Draw();
    explosionEffect_.Draw();
    hitEffect_.Draw();
    dodgeEffect_.Draw();
}