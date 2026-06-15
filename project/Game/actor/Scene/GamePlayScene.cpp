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
#include <filesystem>

static void CreateObjectFromNode(const LevelObjectData& node, const Object3d* parentObj, std::vector<std::unique_ptr<Object3d>>& instances, std::unique_ptr<Rail>& outRail, Object3dCommon* common, uint32_t skyboxTexIndex, std::list<std::unique_ptr<Enemy>>& enemies) {
    const Object3d* currentObj = parentObj;

    if (node.type == "CURVE") {
        if (!outRail) {
            // カーブのポイントはローカル座標なので、オブジェクトのTransformを適用してワールド座標に変換する
            Vector3 rotRad;
            rotRad.x = node.rotation.x * (std::numbers::pi_v<float> / 180.0f);
            rotRad.y = node.rotation.y * (std::numbers::pi_v<float> / 180.0f);
            rotRad.z = node.rotation.z * (std::numbers::pi_v<float> / 180.0f);
            
            Matrix4x4 transformMatrix = Matrix4x4::MakeAffine(node.scale, rotRad, node.translation);
            
            auto TransformVector = [&transformMatrix](const Vector3& v) -> Vector3 {
                return {
                    v.x * transformMatrix.m[0][0] + v.y * transformMatrix.m[1][0] + v.z * transformMatrix.m[2][0] + transformMatrix.m[3][0],
                    v.x * transformMatrix.m[0][1] + v.y * transformMatrix.m[1][1] + v.z * transformMatrix.m[2][1] + transformMatrix.m[3][1],
                    v.x * transformMatrix.m[0][2] + v.y * transformMatrix.m[1][2] + v.z * transformMatrix.m[2][2] + transformMatrix.m[3][2]
                };
            };
            
            std::vector<LevelCurvePoint> worldPoints = node.curvePoints;
            for (auto& pt : worldPoints) {
                pt.position = TransformVector(pt.position);
                pt.handle_left = TransformVector(pt.handle_left);
                pt.handle_right = TransformVector(pt.handle_right);
            }
            
            outRail = std::make_unique<Rail>();
            outRail->Initialize(worldPoints);
        }
    }

    if (node.type == "EMPTY" && !node.fileName.empty()) {
        if (node.fileName == "Fighter" || node.fileName == "Asteroid" || node.fileName.find("Enemy") != std::string::npos || node.fileName.find("Obstacle") != std::string::npos) {
            auto enemy = std::make_unique<Enemy>();
            enemy->Initialize(common, node.translation, node.fileName, skyboxTexIndex);
            enemies.push_back(std::move(enemy));
        }
    }

    if (node.type == "MESH") {
        auto obj = std::make_unique<Object3d>();
        obj->Initialize(common);
        
        std::string modelName = node.fileName;
        if (modelName.empty()) {
            modelName = node.name + ".obj";
        }
        
        // ロードされていない可能性を考慮
        ModelManager::GetInstance()->LoadModel(modelName);
        if (ModelManager::GetInstance()->FindModel(modelName) != nullptr) {
            obj->SetModel(modelName);
            // 黒落ちを回避するために色とテクスチャを設定
            obj->GetModel()->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
            obj->GetModel()->SetTextureIndex(TextureManager::GetInstance()->GetTextureIndexByFilePath("white.png"));
            obj->SetEnvironmentCoefficient(0.0f);
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
        
        if (parentObj) {
            obj->SetParent(parentObj);
        }
        
        currentObj = obj.get();
        instances.push_back(std::move(obj));
    }

    // 子オブジェクトを再帰的に生成
    for (const auto& child : node.children) {
        CreateObjectFromNode(child, currentObj, instances, outRail, common, skyboxTexIndex, enemies);
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
    skybox_->Initialize(dxCommon,"resources/skybox.dds");

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

    particleEffect_.Initialize(dxCommon, srvManager);
    
    //// 最初からデフォルトのノードを追加しておく
    //particleEffect_.AddNode("HitEffect", 0);
    //particleEffect_.AddNode("Shockwave", 1);
    //particleEffect_.AddNode("Aura", 2);
    //
    //particleEffect_.Play();


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
        auto obj = std::make_unique<Object3d>();
        obj->Initialize(object3dCommon);
        obj->SetModel("suzanne.obj");
        obj->GetModel()->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
        uint32_t skyboxTexIndex = skybox_->GetCubemapSrvIndex();
        obj->SetEnvironmentTextureIndex(skyboxTexIndex);
        obj->SetEnvironmentCoefficient(1.0f);
        obj->SetTranslate(Vector3(0.0f, 3.0f, 40.0f));
        obj->SetRotation(Vector3(0.0f, 2.3f, 0.0f));
        obj->SetScale(Vector3(3.0f, 3.0f, 3.0f));
        modelInstances.push_back(std::move(obj));

        auto terrain = std::make_unique<Object3d>();
        terrain->Initialize(object3dCommon);
        terrain->SetModel("terrain.obj");
        terrain->SetTranslate(Vector3(0.0f, -3.0f, 0.0f));
        terrain->SetRotation(Vector3(0.0f, 0.0f, 0.0f));
        terrain->SetScale(Vector3(1000.0f, 1000.0f, 1000.0f));
        modelInstances.push_back(std::move(terrain));
    }

    // レベルデータの読み込みと配置
    auto levelData = LevelLoader::Load("resources/json/maps/template/template.json");
    if (levelData) {
        for (const auto& objData : levelData->objects) {
            CreateObjectFromNode(objData, nullptr, modelInstances, mainRail_, object3dCommon, skybox_->GetCubemapSrvIndex(), enemies_);
        }
        OutputDebugStringA("LevelLoader: Successfully placed objects.\n");
    } else {
        OutputDebugStringA("LevelLoader: Failed to load level.\n");
    }

    // レールの可視化用オブジェクトの生成
    if (mainRail_ && mainRail_->IsValid()) {
        railModel_ = std::make_unique<Model>();
        
        std::string resolved = ResourceLocator::Resolve("rail.obj", ResourceLocator::AssetType::Model3D);
        if (!resolved.empty()) {
            std::filesystem::path rp(reinterpret_cast<const char8_t*>(resolved.c_str()));
            std::string directory = rp.parent_path().string();
            std::string filename = rp.filename().string();
            railModel_->Initialize(ModelManager::GetInstance()->GetModelCommon(), directory, filename);
        } else {
            // フォールバック
            railModel_->Initialize(ModelManager::GetInstance()->GetModelCommon(), "resources/models", "rail.obj");
        }
        
        railModel_->SetColor({1.0f, 0.0f, 0.0f, 1.0f}); // 独立した赤色のマテリアル

        int sampleCount = 200; // 分割数を増やして滑らかな線にする
        for (int i = 0; i < sampleCount; ++i) {
            float t1 = static_cast<float>(i) / sampleCount;
            float t2 = static_cast<float>(i + 1) / sampleCount;
            
            Vector3 p1 = mainRail_->GetPosition(t1);
            Vector3 p2 = mainRail_->GetPosition(t2);
            
            Vector3 dir = { p2.x - p1.x, p2.y - p1.y, p2.z - p1.z };
            float length = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
            if (length < 0.0001f) continue;
            
            dir.x /= length; dir.y /= length; dir.z /= length;
            
            Vector3 center = { (p1.x + p2.x) * 0.5f, (p1.y + p2.y) * 0.5f, (p1.z + p2.z) * 0.5f };
            
            auto obj = std::make_unique<Object3d>();
            obj->Initialize(object3dCommon);
            obj->SetModel(railModel_.get()); 
            obj->SetTranslate(center);
            float yaw = std::atan2(dir.x, dir.z);
            float pitch = std::asin(-dir.y);
            obj->SetRotation(Vector3(pitch, yaw, 0.0f));
            
            // 少し長めにスケールして途切れないようにする(length)
            obj->SetScale(Vector3(0.02f, 0.02f, length)); 
            
            obj->SetEnvironmentCoefficient(0.0f);
            obj->SetEnvironmentTextureIndex(skybox_->GetCubemapSrvIndex());
            
            railVisualizers_.push_back(std::move(obj));
        }
    }

    // カメラオブジェクト（プレイヤーの親となる）の初期化
    cameraObject_ = std::make_unique<Object3d>();
    cameraObject_->Initialize(object3dCommon);

    // プレイヤーの初期化
    player_ = std::make_unique<Player>();
    player_->Initialize(object3dCommon, skybox_->GetCubemapSrvIndex());
    player_->LoadSettings("resources/json/player/player_settings.json");
    
    // プレイヤーをカメラオブジェクトの子にする
    player_->GetObject3d()->SetParent(cameraObject_.get());
    player_->GetReticle()->SetParent(cameraObject_.get());

    // 全てのモデル・テクスチャ読み込みが終わった後にGPUへ転送する
    texManager->ExecuteUploadCommands();
    texManager->ClearIntermediateResources();
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

namespace {
    float LerpAngle(float a, float b, float t) {
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

    // --- カメラ切り替え ---
    if (isPlaying_) {
        activeCamera_ = camera.get();
    } else {
        activeCamera_ = debugCamera_.get();
    }
    if (auto objCommon = services->GetObject3dCommon()) {
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
            float moveStep = kMoveSpeed * kDeltaTime_;
            Vector3 pos = activeCamera_->GetTranslate();
            Vector3 rot = activeCamera_->GetRotation();
            float yaw = rot.y;

            // カメラの向きから forward / right を構成
            Vector3 forward = { std::sinf(yaw), 0.0f, std::cosf(yaw) };
            Vector3 right = { std::cosf(yaw), 0.0f, -std::sinf(yaw) };

            auto normalize = [](Vector3 v) {
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
            if (mainRail_ && mainRail_->IsValid()) {
                float kAutoSpeed = 0.002f * gameSpeed_; // レール上の進行速度(要調整)
                railProgress_ += kAutoSpeed;
                if (railProgress_ > 1.0f) {
                    railProgress_ = 1.0f; // 終点で停止
                }

                // カメラをレールから「上へ1.0f」浮かせる
                // （回転行列でローカルの上方向にずらすと、曲線の接線変化で位置がブレてガタつくため、ワールド座標で単純に上にずらす）
                Vector3 baseEye = mainRail_->GetPosition(railProgress_);
                Vector3 eye = baseEye;
                eye.y += 0.2f;
                
                // 注視点は少し進んだ地点（厳密にレールに沿わせるため距離を短くする）
                float targetProgress = std::min(railProgress_ + 0.01f, 1.0f);
                Vector3 baseTarget = mainRail_->GetPosition(targetProgress);

                // 差分ベクトル (forward)
                Vector3 forward = {
                    baseTarget.x - baseEye.x,
                    baseTarget.y - baseEye.y,
                    baseTarget.z - baseEye.z
                };

                // 正規化
                float len = std::sqrtf(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
                if (len > 1e-6f) {
                    forward.x /= len; forward.y /= len; forward.z /= len;
                }

                // 目標の回転角の計算（レールの接線にピッタリ合わせる）
                float targetYaw = std::atan2(forward.x, forward.z);
                float targetPitch = std::asin(-forward.y);
                float railTilt = mainRail_->GetTilt(railProgress_);

                // 補間なしで完全にレールと同じ向きにする
                Vector3 cameraRot(targetPitch, targetYaw, railTilt);

                // カメラオブジェクトのワールド行列に反映（自機などの親）
                if (cameraObject_) {
                    cameraObject_->SetTranslate(eye);
                    cameraObject_->SetRotation(cameraRot);
                    cameraObject_->Update();
                }

                // 実際のカメラの更新 (スライド4の通り、カメラオブジェクトのワールド行列の逆行列をビュー行列とするため、完全に一致させる)
                activeCamera_->SetTranslate(eye);
                activeCamera_->SetRotation(cameraRot);
            } else {
                // レールが無い場合のフォールバック（自動前進）
                const float kAutoSpeed = 0.05f;
                Vector3 camPos = activeCamera_->GetTranslate();
                camPos.z += kAutoSpeed;
                activeCamera_->SetTranslate(camPos);

                if (cameraObject_) {
                    cameraObject_->SetTranslate(camPos);
                    cameraObject_->Update();
                }
            }
        }
    }

    // プレイヤーの更新
    if (player_) {
        if (isPlaying_) {
            player_->Update(bullets_, cameraObject_.get());
        } else {
            // ゲーム停止中でも、Object3dの更新(カメラ行列の反映など)は必要
            player_->Update3DObjectOnly();
        }
    }

    // 弾の更新
    if (isPlaying_) {
        for (auto it = bullets_.begin(); it != bullets_.end(); ) {
            (*it)->Update();
            if ((*it)->IsDead()) {
                it = bullets_.erase(it);
            } else {
                ++it;
            }
        }

        // 敵の更新と当たり判定
        for (auto it = enemies_.begin(); it != enemies_.end();) {
            (*it)->Update();

            // プレイヤーの弾との当たり判定
            for (auto& bullet : bullets_) {
                if (bullet->IsDead()) continue;

                Vector3 diff = {
                    (*it)->GetPosition().x - bullet->GetPosition().x,
                    (*it)->GetPosition().y - bullet->GetPosition().y,
                    (*it)->GetPosition().z - bullet->GetPosition().z
                };
                float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                // 弾の半径をとりあえず1.0fとする
                float rSq = ((*it)->GetRadius() + 1.0f) * ((*it)->GetRadius() + 1.0f);

                if (distSq <= rSq) {
                    bullet->OnCollision();
                    (*it)->OnCollision();
                    
                    // パーティクルの再生
                    particleEffect_.Play();
                }
            }

            if ((*it)->IsDead()) {
                // 破壊エフェクト
                particleEffect_.Play();
                it = enemies_.erase(it);
            } else {
                ++it;
            }
        }

        // 当たり判定で死んだ弾を削除
        for (auto it = bullets_.begin(); it != bullets_.end(); ) {
            if ((*it)->IsDead()) {
                it = bullets_.erase(it);
            } else {
                ++it;
            }
        }

    } else {
        for (auto& bullet : bullets_) {
            bullet->Update3DObjectOnly();
        }
        for (auto& enemy : enemies_) {
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
    if (isDrawRail_) {
        for (auto& vis : railVisualizers_) if (vis) vis->Update();
    }

    // カメラ行列の取得
    Matrix4x4 cameraMatrix = Matrix4x4::Identity();
    Matrix4x4 viewMatrix = Matrix4x4::Identity();
    Matrix4x4 projectionMatrix = Matrix4x4::Identity();
    Matrix4x4 billboardMatrix = Matrix4x4::Identity();

    if (activeCamera_) {
        cameraMatrix = activeCamera_->GetWorldMatrix();
        viewMatrix = activeCamera_->GetViewMatrix();
        projectionMatrix = activeCamera_->GetProjectionMatrix();
        billboardMatrix = Billboard::CreateFromCamera(activeCamera_, true);
    }

    if (skybox_) {
        skybox_->SetCamera(activeCamera_);
        skybox_->Update();
    }

    if (isPlaying_)
    {
        particleEffect_.Update(kDeltaTime_, viewMatrix, projectionMatrix, billboardMatrix);
    }

#ifdef USE_IMGUI

    ImGui::Begin("Game Control");
    
    bool doReset = false;

    // シーン切り替えとプレイ状態
    if (isPlaying_) {
        if (ImGui::Button("Stop (Pause)", ImVec2(120, 40))) {
            isPlaying_ = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset", ImVec2(120, 40))) {
            doReset = true;
        }
        ImGui::Text("Status: PLAYING");
    } else {
        if (ImGui::Button("Play (Start)", ImVec2(120, 40))) {
            isPlaying_ = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset", ImVec2(120, 40))) {
            doReset = true;
        }
        ImGui::Text("Status: STOPPED (Free Camera Mode)");
        ImGui::Text("Camera Control: WASD/QE to move, Right-Click Drag to rotate");
    }

    ImGui::Separator();
    if (ImGui::SliderFloat("ゲーム時間 (Rail Progress)", &railProgress_, 0.0f, 1.0f)) {
        if (!isPlaying_ && mainRail_ && mainRail_->IsValid()) {
            Vector3 baseEye = mainRail_->GetPosition(railProgress_);
            float targetProgress = std::min(railProgress_ + 0.01f, 1.0f);
            Vector3 baseTarget = mainRail_->GetPosition(targetProgress);

            Vector3 forward = {
                baseTarget.x - baseEye.x,
                baseTarget.y - baseEye.y,
                baseTarget.z - baseEye.z
            };
            float len = std::sqrtf(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
            if (len > 1e-6f) {
                forward.x /= len; forward.y /= len; forward.z /= len;
            }

            float targetYaw = std::atan2(forward.x, forward.z);
            float targetPitch = std::asin(-forward.y);
            float railTilt = mainRail_->GetTilt(railProgress_);
            Vector3 cameraRot(targetPitch, targetYaw, railTilt);

            if (cameraObject_) {
                cameraObject_->SetTranslate(baseEye);
                cameraObject_->SetRotation(cameraRot);
                cameraObject_->Update();
            }
            if (camera) {
                camera->SetTranslate(baseEye);
                camera->SetRotation(cameraRot);
            }
        }
    }
    ImGui::SliderFloat("ゲームスピード (Game Speed)", &gameSpeed_, 0.0f, 5.0f);

    // リセット処理：レール進行度を0に戻し、カメラとプレイヤーを始点に移動させる
    if (doReset) {
        railProgress_ = 0.0f;
        isPlaying_ = false;
        if (mainRail_ && mainRail_->IsValid()) {
            Vector3 baseEye = mainRail_->GetPosition(0.0f);
            Vector3 railForward = mainRail_->GetForward(0.0f);
            float railTilt = mainRail_->GetTilt(0.0f);
            float yaw = std::atan2(railForward.x, railForward.z);
            float pitch = std::asin(-railForward.y);
            Vector3 cameraRot(pitch, yaw, railTilt);

            // リセット時に補間用変数を初期化
            currentCameraRot_ = {pitch, yaw, 0.0f};
            lastCameraYaw_ = yaw;
            currentCameraBank_ = railTilt;

            // カメラをレールから「上へ1.0f」浮かせる（ワールド座標で単純に上にずらす）
            Vector3 eye = baseEye;
            eye.y += 1.0f;

            if (activeCamera_) {
                activeCamera_->SetTranslate(eye);
                activeCamera_->SetRotation(cameraRot);
            }
            if (cameraObject_) {
                cameraObject_->SetTranslate(eye);
                cameraObject_->SetRotation(cameraRot);
                cameraObject_->Update();
            }
        }
    }
    
    ImGui::Separator();
    ImGui::Checkbox("レールを表示 (Draw Rail)", &isDrawRail_);
    ImGui::End();

    if (player_) {
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
    for (auto& obj : modelInstances) {
        if (obj) {
            allModels.push_back(obj.get());
            modelNames.push_back("Model " + std::to_string(index));
        }
        index++;
    }
    if (player_) {
        if (player_->GetObject3d()) {
            allModels.push_back(player_->GetObject3d());
            modelNames.push_back("Player");
        }
        if (player_->GetReticle()) {
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
        for (const auto& name : modelNames) {
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
        auto usesDirectional = [](int mode) {
            return mode == 1 || mode == 2 || mode == 3 || mode == 4;
            };
        auto usesPoint = [](int mode) {
            return mode == 4;
            };
        auto usesSpot = [](int mode) {
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
    particleEffect_.DrawImGui();


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

    if (object3dCommon) object3dCommon->SetCommonDrawSetting();

    for (auto& model : modelInstances) if (model) model->Draw();

    if (spriteCommon) spriteCommon->SetCommonDrawSetting();

    if (isDisplaySprite)
    {
        for (auto& sprite : sprites) if (sprite) sprite->Draw();
    }

    // 弾の描画
    for (auto& bullet : bullets_) {
        bullet->Draw();
    }
    
    // 敵の描画
    if (object3dCommon) object3dCommon->SetCommonDrawSetting();
    for (auto& enemy : enemies_) {
        enemy->Draw();
    }
    
    // プレイヤーの描画
    if (player_) {
        if (object3dCommon) object3dCommon->SetCommonDrawSetting();
        player_->Draw();
    }

    // レールの描画
    if (isDrawRail_) {
        if (object3dCommon) object3dCommon->SetCommonDrawSetting();
        for (auto& vis : railVisualizers_) {
            if (vis) vis->Draw();
        }
    }

    particleEffect_.Draw();
}