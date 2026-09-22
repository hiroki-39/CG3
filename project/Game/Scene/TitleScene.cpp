#define NOMINMAX
#include "TitleScene.h"
#include "KHEngine/Core/Services/EngineServices.h"
#include "KHEngine/Graphics/Resource/Texture/TextureManager.h"
#include "KHEngine/Graphics/3d/Model/ModelManager.h"
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include "KHEngine/Graphics/2d/SpriteCommon.h"
#include "KHEngine/Scene/SceneManager.h"
#include "KHEngine/Debug/Editor/EffectStudio.h"
#include "KHEngine/Graphics/PostProcess/PostProcess.h"
#include "externals/imgui/imgui.h"
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
        if (sceneManager && !sceneManager->IsTransitioning())
        {
            sceneManager->ChangeScene("GAMEPLAY", 0.6f);
        }
    }

#ifdef USE_IMGUI
    // フレーム描画スコープ外（NewFrame前）の場合はImGui描画をスキップ
    if (!ImGui::GetCurrentContext() || ImGui::GetFrameCount() <= 0)
    {
        return;
    }

    // =========================================================================
    // 統合エディタUI: [左] メニュー  [右] インスペクター  [下] タイムライン
    // (GamePlaySceneと完全に同じドッキング配置を維持)
    // =========================================================================
    static int currentNavIndex = 0;
    const char* navItems[] = {
        "ゲーム・進行",
        "演出・シェーダー"
    };

    // -------------------------------------------------------------
    // 1. [左ドック用] メニュー (Category Selector & Performance)
    // -------------------------------------------------------------
    if (ImGui::Begin("メニュー"))
    {
        ImGui::TextColored(ImVec4(0.3f, 0.75f, 1.0f, 1.0f), "CATEGORY");
        ImGui::Separator();

        for (int i = 0; i < IM_ARRAYSIZE(navItems); ++i)
        {
            bool isSelected = (currentNavIndex == i);
            if (ImGui::Selectable(navItems[i], isSelected, 0, ImVec2(0, 30)))
            {
                currentNavIndex = i;
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextDisabled("F1: UI表示切替");
        if (ImGui::Button("ゲームプレイ開始 (SPACE)", ImVec2(-1, 32)))
        {
            auto sceneManager = GetSceneManager();
            if (sceneManager && !sceneManager->IsTransitioning())
            {
                sceneManager->ChangeScene("GAMEPLAY", 0.6f);
            }
        }

        // 最下部にパフォーマンス情報を常時表示
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.3f, 0.85f, 0.5f, 1.0f), "PERFORMANCE");

        float fps = ImGui::GetIO().Framerate;
        float frameTime = 1000.0f / (fps > 0.0f ? fps : 1.0f);

        ImGui::Text("FPS: %.1f (%.2f ms)", fps, frameTime);

        static float fpsHistory[60] = {};
        static int historyOffset = 0;
        fpsHistory[historyOffset] = fps;
        historyOffset = (historyOffset + 1) % IM_ARRAYSIZE(fpsHistory);

        ImGui::PlotLines("##FPSMiniGraphTitle", fpsHistory, IM_ARRAYSIZE(fpsHistory), historyOffset, nullptr, 0.0f, 120.0f, ImVec2(-1, 42));
    }
    ImGui::End();

    // -------------------------------------------------------------
    // 2. [右ドック用] インスペクター (Inspector Content)
    // -------------------------------------------------------------
    if (ImGui::Begin("インスペクター"))
    {
        // 1. ゲーム・進行
        if (currentNavIndex == 0)
        {
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "■ シーン・進行");
            ImGui::Separator();

            if (ImGui::CollapsingHeader("シーン切り替え (Scene Selector)", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (auto sm = GetSceneManager())
                {
                    sm->DrawSceneSelectorUI();
                }
            }

            ImGui::Spacing();
            if (ImGui::CollapsingHeader("タイトル画面制御 (Title Control)", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("現在: タイトル画面 (TITLE SCENE)");
                ImGui::TextDisabled("SPACEキーを押すか、下のボタンでゲームを開始できます。");
                ImGui::Spacing();

                if (ImGui::Button("ゲームプレイを開始する (Start Game)", ImVec2(-1, 40)))
                {
                    auto sm = GetSceneManager();
                    if (sm && !sm->IsTransitioning())
                    {
                        sm->ChangeScene("GAMEPLAY", 0.6f);
                    }
                }
            }
        }
        // 2. 演出・シェーダー
        else if (currentNavIndex == 1)
        {
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "■ 演出・トランジション・シェーダー");
            ImGui::Separator();

            if (ImGui::CollapsingHeader("画面遷移 (Transition Settings)", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (auto sm = GetSceneManager())
                {
                    sm->DrawTransitionSettingsUI();
                }
            }

            ImGui::Spacing();
            if (ImGui::CollapsingHeader("ポストプロセス (Post Process)", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (auto pp = EngineServices::GetInstance()->GetPostProcess())
                {
                    pp->DrawImGuiContent();
                }
            }

            ImGui::Spacing();
            if (ImGui::CollapsingHeader("パーティクルエフェクト (Particles)", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::Button("エフェクト専用画面 (Effect Studio) を開く", ImVec2(-1, 36)))
                {
                    EffectStudio::GetInstance()->GetShowViewport() = true;
                    EffectStudio::GetInstance()->GetShowEditor() = true;
                }
                ImGui::TextDisabled("※エフェクトの編集・作成・保存は「エフェクト画面」および「エフェクトエディター」で行えます。");
            }
        }
    }
    ImGui::End();

    // -------------------------------------------------------------
    // 3. [下ドック用] タイムライン (Timeline Editor)
    // -------------------------------------------------------------
    if (ImGui::Begin("タイムライン"))
    {
        ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "■ タイムライン (Timeline)");
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextDisabled("※ 現在はタイトルシーンです。レールや敵出現タイムラインの編集・確認は「ゲームプレイ画面」で行えます。");
        ImGui::Spacing();
        if (ImGui::Button("ゲームプレイ画面へ移動してタイムラインを編集##TLTitle", ImVec2(340, 36)))
        {
            auto sm = GetSceneManager();
            if (sm && !sm->IsTransitioning())
            {
                sm->ChangeScene("GAMEPLAY", 0.6f);
            }
        }
    }
    ImGui::End();
#endif
}

void TitleScene::Draw()
{
}

void TitleScene::Finalize()
{
    
    BaseScene::Finalize();
}
