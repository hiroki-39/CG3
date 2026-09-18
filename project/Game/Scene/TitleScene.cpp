#define NOMINMAX
#include "TitleScene.h"
#include "KHEngine/Core/Services/EngineServices.h"
#include "KHEngine/Graphics/Resource/Texture/TextureManager.h"
#include "KHEngine/Graphics/3d/Model/ModelManager.h"
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include "KHEngine/Graphics/2d/SpriteCommon.h"
#include "KHEngine/Scene/SceneManager.h"
#include <limits>
#include <memory>
#include <Windows.h>

void TitleScene::Initialize()
{
    
    auto services = Services();
    if (!services) return;

    auto dxCommon = services->GetDirectXCommon();
    auto spriteCommon = services->GetSpriteCommon();
    auto texManager = TextureManager::GetInstance();
    if (!texManager) return;

    
    if (dxCommon) dxCommon->BeginTextureUploadBatch();

    texManager->LoadTexture("monsterBall.png");
    uint32_t monsterTex = texManager->GetTextureIndexByFilePath("monsterBall.png");
    if (spriteCommon && monsterTex != std::numeric_limits<uint32_t>::max())
    {
        auto s = std::make_unique<Sprite>();
        s->Initialize(spriteCommon, monsterTex);
        s->SetPosition(Vector2(400.0f, 300.0f));
        s->SetSize(Vector2(256.0f, 256.0f));
        s->SetAnchorPoint(Vector2(0.5f, 0.5f));
        s->SetColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
        AddSprite(std::move(s)); 
    }

    // -------------------------------------------------------------
    // ゲームプレイ用アセットの事前キャッシュ（プリロード）：
    // シーン遷移時のフリーズ（1.7秒）を解消し、瞬時遷移を実現する
    // -------------------------------------------------------------
    OutputDebugStringA("\n[Preload] Starting background preload for GamePlayScene...\n");
    auto modelManager = ModelManager::GetInstance();
    if (modelManager)
    {
        // 1. 地形・背景モデル（約15MBの巨大OBJをタイトル起動時にパース・キャッシュ）
        modelManager->LoadModel("AITerrain_00_OPEN_SEA.obj");
        modelManager->LoadModel("AITerrain_01_PLAINS.obj");
        modelManager->LoadModel("AITerrain_02_HILLS.obj");
        modelManager->LoadModel("AITerrain_03_CANYON.obj");
        modelManager->LoadModel("AITerrain_Background_Mountains.obj");
        modelManager->LoadModel("pillar.obj");
        modelManager->LoadModel("wall.obj");

        // 2. プレイヤー・敵・弾・障害物モデル
        modelManager->LoadModel("player.obj");
        modelManager->LoadModel("cube.obj");
        modelManager->LoadModel("plane.obj");
        modelManager->LoadModel("Fighter.obj");
        modelManager->LoadModel("missile.obj");
        modelManager->LoadModel("RockOn.obj");
        modelManager->LoadModel("collider_cube_player.obj");
        modelManager->LoadModel("collider_sphere_enemy.obj");
        modelManager->LoadModel("collider_cube_enemy.obj");
        modelManager->LoadModel("Ring.obj");
        modelManager->LoadModel("heal.obj");
        modelManager->LoadModel("rail.obj");
    }

    // 3. スカイボックス（26.7MB）および主要テクスチャの事前ロード
    texManager->LoadTexture("resources/skybox.dds");
    texManager->LoadTexture("uvChecker.png");
    texManager->LoadTexture("circle.png");
    texManager->LoadTexture("circle2.png");
    texManager->LoadTexture("gradationLine.png");
    texManager->LoadTexture("sprites/white.png");
    texManager->LoadTexture("sprites/prticle_kira.png");
    texManager->LoadTexture("sprites/hart.png");

    // 全テクスチャ（モデル用・スカイボックス用含む）を一括アップロード
    texManager->ExecuteUploadCommands();
    texManager->ClearIntermediateResources();
    OutputDebugStringA("[Preload] Preload completed! GamePlayScene will now start instantly.\n\n");
}

void TitleScene::Update()
{
    
    UpdateSprites();

    
    auto services = Services();
    if (!services) return;

    auto input = services->GetInput();
    if (input && input->TriggerKey(DIK_SPACE))
    {
        auto sceneManager = GetSceneManager();
        if (sceneManager)
        {
            sceneManager->ChangeScene("GAMEPLAY");
        }
    }
}

void TitleScene::Draw()
{
}

void TitleScene::Finalize()
{
    
    BaseScene::Finalize();
}
