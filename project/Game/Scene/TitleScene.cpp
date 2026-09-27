#define NOMINMAX
#include "TitleScene.h"
#include "KHEngine/Core/Services/EngineServices.h"
#include "KHEngine/Graphics/Resource/Texture/TextureManager.h"
#include "KHEngine/Graphics/Resource/Descriptor/SrvManager.h"
#include "KHEngine/Graphics/3d/Model/ModelManager.h"
#include "KHEngine/Graphics/3d/Object/Object3dCommon.h"
#include "KHEngine/Graphics/3d/Particle/ParticleManager.h"
#include "KHEngine/Graphics/Billboard/Billboard.h"
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include "KHEngine/Graphics/2d/SpriteCommon.h"
#include "KHEngine/Scene/SceneManager.h"
#ifdef ENABLE_EDITOR
#include "KHEngine/Debug/Editor/EffectStudio.h"
#endif
#include "KHEngine/Graphics/PostProcess/PostProcess.h"
#include "KHEngine/Sound/Core/SoundManager.h"
#include "externals/imgui/imgui.h"
#include "externals/nlohmann/json.hpp"
#include <limits>
#include <memory>
#include <fstream>
#include <cmath>
#include <numbers>
#include <Windows.h>

void TitleScene::Initialize()
{
    auto services = Services();
    if (!services) return;

    auto dxCommon = services->GetDirectXCommon();
    auto object3dCommon = services->GetObject3dCommon();
    auto srvManager = services->GetSrvManager();
    auto texManager = TextureManager::GetInstance();
    if (!texManager) return;

    if (dxCommon) dxCommon->BeginTextureUploadBatch();

    // -------------------------------------------------------------
    // 3Dカメラの初期化
    // -------------------------------------------------------------
    camera_ = std::make_unique<Camera>();
    camera_->SetTranslate(cameraPos_);
    camera_->SetRotation(cameraRot_);
    if (object3dCommon)
    {
        object3dCommon->SetDefaultCamera(camera_.get());
    }

    // -------------------------------------------------------------
    // スカイボックスの初期化
    // -------------------------------------------------------------
    skybox_ = std::make_unique<Skybox>();
    skybox_->Initialize(dxCommon, "resources/skybox.dds");

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
        modelManager->LoadModel("beam.obj");
    }

    // 3. スカイボックス（26.7MB）および主要テクスチャの事前ロード
    texManager->LoadTexture("resources/skybox.dds");
    texManager->LoadTexture("uvChecker.png");
    texManager->LoadTexture("circle.png");
    texManager->LoadTexture("circle2.png");
    texManager->LoadTexture("gradationLine.png");
    texManager->LoadTexture("sprites/white.png");
    texManager->LoadTexture("prticle_kira.png");
    texManager->LoadTexture("hart.png");
    texManager->LoadTexture("light.png");
    texManager->LoadTexture("sprites/UI/ringGet_icon_outline.png");
    texManager->LoadTexture("sprites/UI/ringGet_icon.png");

    // 全テクスチャ（モデル用・スカイボックス用含む）を一括アップロード
    texManager->ExecuteUploadCommands();
    texManager->ClearIntermediateResources();
    OutputDebugStringA("[Preload] Preload completed! GamePlayScene will now start instantly.\n\n");

    // -------------------------------------------------------------
    // タイトル自機の生成と初期化
    // -------------------------------------------------------------
    if (object3dCommon)
    {
        playerObj_ = std::make_unique<Object3d>();
        playerObj_->Initialize(object3dCommon);
        playerObj_->SetModel("player.obj");

        // プレイヤー設定ファイルからスケールとカラーを読み込み
        std::ifstream file("resources/json/player/player_settings.json");
        if (file.is_open())
        {
            try
            {
                nlohmann::json j;
                file >> j;
                if (j.contains("playerScale"))
                {
                    auto s = j["playerScale"];
                    if (s.is_array() && s.size() == 3)
                    {
                        playerScale_ = { s[0], s[1], s[2] };
                    }
                }
                if (j.contains("color"))
                {
                    auto c = j["color"];
                    if (c.is_array() && c.size() == 4)
                    {
                        playerColor_ = { c[0], c[1], c[2], c[3] };
                    }
                }
            }
            catch (...) {}
            file.close();
        }

        playerObj_->SetScale(playerScale_);
        playerObj_->SetColor(playerColor_);
        if (skybox_)
        {
            playerObj_->SetEnvironmentTextureIndex(skybox_->GetCubemapSrvIndex());
        }
        playerObj_->SetEnvironmentCoefficient(envCoefficient_);
        playerObj_->SetEnableLighting(true);

        // 登場前進演出の初期位置に配置
        playerPos_ = startPos_;
        playerRot_ = { 0.0f, 0.0f, 0.0f }; // 回転はすべて 0
        playerObj_->SetTranslate(playerPos_);
        playerObj_->SetRotation(playerRot_);
        playerObj_->Update();
    }

    // -------------------------------------------------------------
    // ブースト・気流粒子パーティクルエフェクトの初期化
    // -------------------------------------------------------------
    if (srvManager)
    {
        ParticleManager::GetInstance()->RegisterQuad("quad", "circle2.png");
        ParticleManager::GetInstance()->RegisterRing("ring", "resources/sprites/effect/gradationLine.png", 32, 0.5f, 1.0f);
        ParticleManager::GetInstance()->RegisterCylinder("Cylinder", "resources/sprites/effect/gradationLine.png");

        thrusterEffect_.Initialize(dxCommon, srvManager);
        thrusterEffect_.LoadFromJson("thruster.json");

        windEffect_.Initialize(dxCommon, srvManager);
        windEffect_.LoadFromJson("wind.json");

        randomEngine_.seed(1337);
    }

    // 登場演出タイマーの初期化
    isIntro_ = true;
    introTimer_ = 0.0f;
    isDiving_ = false;
    diveTimer_ = 0.0f;

    // タイトルBGMの再生（ループ再生、音量0.25）
    SoundManager::GetInstance()->PlayBGM("tileBGM.mp3", 0.25f, true);
}

void TitleScene::Update()
{
    UpdateSprites();

    auto services = Services();
    if (!services) return;

    auto input = services->GetInput();

    // -------------------------------------------------------------
    // 決定ボタン押下時の判定（自機を傾けて急降下しゲーム開始）
    // -------------------------------------------------------------
    bool isDecisionPressed = false;
    if (input)
    {
        if (input->TriggerKey(DIK_SPACE) || input->TriggerKey(DIK_RETURN) ||
            input->TriggerPadButton(XINPUT_GAMEPAD_A) || input->TriggerPadButton(XINPUT_GAMEPAD_START))
        {
            isDecisionPressed = true;
        }
    }

    if (isDecisionPressed && !isDiving_)
    {
        isDiving_ = true;
        diveTimer_ = 0.0f;
        diveStartPos_ = playerPos_;
        diveStartRot_ = playerRot_;

        // 決定SE再生
        SoundManager::GetInstance()->PlaySE("select.mp3", 0.9f);

        auto sceneManager = GetSceneManager();
        if (sceneManager && !sceneManager->IsTransitioning())
        {
            // 降下演出に合わせてシーン遷移（diveDuration_）
            sceneManager->ChangeScene("GAMEPLAY", diveDuration_);
        }
    }

    // -------------------------------------------------------------
    // 自機のアニメーション（降下演出 / 前進登場演出 / アイドルホバリング）
    // -------------------------------------------------------------
    const float dt = 1.0f / 60.0f;
    idleTimer_ += dt;

    Vector3 curPos = targetPos_;
    Vector3 curRot = playerRot_; // 基本回転はすべて 0

    if (isDiving_)
    {
        // === 決定後の急降下演出（回転はZ軸のみ！右回転して腹を見せながら右へ大回りして下降、カメラ固定） ===
        diveTimer_ += dt;
        float t = std::clamp(diveTimer_ / diveDuration_, 0.0f, 1.0f);

        // 最初の0.5秒でZ軸のみ右回転（SmoothStepで滑らかに半回転し腹面をカメラ側へ向ける）
        float rollProgress = std::clamp(diveTimer_ / 0.5f, 0.0f, 1.0f);
        float rollEase = rollProgress * rollProgress * (3.0f - 2.0f * rollProgress); // SmoothStep

        // ★回転はZ軸のみ！X（ピッチ）とY（ヨー）は完全に0★
        curRot.x = 0.0f;
        curRot.y = 0.0f;
        curRot.z = diveStartRot_.z + diveRollAngle_ * rollEase; // 右回転（-180度半回転して完全に腹を見せる）

        // === 大回り軌道（Wide Arc）の計算 ===
        // X方向: 右側(+X)へ大きく弧を描いて膨らみ、画面をダイナミックに大回りする
        float arcX = std::sinf(t * 3.14159265f * 0.85f) * diveArcWidthX_ + (t * t) * 3.5f;
        // Y方向: 序盤は落下を抑えて横への大きな回頭を見せ、後半にかけて一気に加速急降下
        float dropProgress = std::powf(t, 1.7f);
        // Z方向: 奥方向へ加速前進
        float forwardProgress = t * t;

        curPos.x = diveStartPos_.x + arcX;
        curPos.y = diveStartPos_.y - diveDropDistanceY_ * dropProgress;
        curPos.z = diveStartPos_.z + diveForwardDistanceZ_ * forwardProgress;

        // ※カメラは完全に固定（一切動かさない）
    }
    else if (isIntro_)
    {
        // === 起動時の手前から定位置への前進登場演出 ===
        introTimer_ += dt;
        float t = std::clamp(introTimer_ / introDuration_, 0.0f, 1.0f);

        // Cubic Ease-Out による滑らかな前進・定位置への減速停止
        float easeOut = 1.0f - std::powf(1.0f - t, 3.0f);
        curPos.x = startPos_.x + (targetPos_.x - startPos_.x) * easeOut;
        curPos.y = startPos_.y + (targetPos_.y - startPos_.y) * easeOut;
        curPos.z = startPos_.z + (targetPos_.z - startPos_.z) * easeOut;

        // 前進加速中はわずかに機首が上がるリアリティ演出
        float pitchAngle = std::sinf(t * 3.14159265f) * -0.04f;
        curRot.x += pitchAngle;

        if (t >= 1.0f)
        {
            isIntro_ = false;
        }
    }
    else
    {
        // === 定位置での巡航・ホバリング演出 ===
        curPos = targetPos_;
        curRot = playerRot_;

        if (isHovering_)
        {
            curPos.y += std::sinf(idleTimer_ * hoverSpeed_) * hoverAmplitude_;
            curRot.z += std::sinf(idleTimer_ * (hoverSpeed_ * 0.8f)) * 0.012f;
        }

        cameraRot_.x = 0.05f;
    }

    playerPos_ = curPos;

    if (playerObj_)
    {
        playerObj_->SetTranslate(curPos);
        playerObj_->SetRotation(curRot);
        playerObj_->SetScale(playerScale_);
        playerObj_->SetColor(playerColor_);
        playerObj_->SetEnvironmentCoefficient(envCoefficient_);
        playerObj_->Update();
    }

    // カメラ更新
    if (camera_)
    {
        camera_->SetTranslate(cameraPos_);
        camera_->SetRotation(cameraRot_);
        camera_->Update();
    }

    // スカイボックス更新
    if (skybox_ && camera_)
    {
        skybox_->SetCamera(camera_.get());
        skybox_->Update();
    }

    // -------------------------------------------------------------
    // ブースト（スラスター）＆周辺粒子（気流線）の更新
    // -------------------------------------------------------------
    if (camera_)
    {
        Matrix4x4 viewMatrix = camera_->GetViewMatrix();
        Matrix4x4 projectionMatrix = camera_->GetProjectionMatrix();
        Matrix4x4 billboardMatrix = Billboard::CreateFromCamera(camera_.get(), true);

        // 1. スラスター（ブースト炎）
        // 自機のロール回転に完全連動した機体後方ノズル座標を計算（nozzleOffset_ でノズル中心にピタッと合致）
        Vector3 nozzlePos = curPos;
        if (playerObj_)
        {
            const Matrix4x4& mat = playerObj_->GetmatWorld();
            nozzlePos = {
                nozzleOffset_.x * mat.m[0][0] + nozzleOffset_.y * mat.m[1][0] + nozzleOffset_.z * mat.m[2][0] + mat.m[3][0],
                nozzleOffset_.x * mat.m[0][1] + nozzleOffset_.y * mat.m[1][1] + nozzleOffset_.z * mat.m[2][1] + mat.m[3][1],
                nozzleOffset_.x * mat.m[0][2] + nozzleOffset_.y * mat.m[1][2] + nozzleOffset_.z * mat.m[2][2] + mat.m[3][2]
            };
        }
        thrusterEffect_.SetPosition(nozzlePos);

        if (isDiving_)
        {
            // 急降下中は高出力バーニングオレンジ
            thrusterEffect_.SetBaseColor({ 1.0f, 0.45f, 0.05f, 1.0f });
        }
        else if (isIntro_)
        {
            thrusterEffect_.SetBaseColor({ 1.0f, 0.35f, 0.05f, 1.0f });
        }
        else
        {
            // 巡航中はクールな高エネルギーシアンブルー
            thrusterEffect_.SetBaseColor({ 0.5f, 0.85f, 1.0f, 1.0f });
        }
        thrusterEffect_.Play();
        thrusterEffect_.Update(dt, viewMatrix, projectionMatrix, billboardMatrix);

        // 2. 周辺気流粒子（スピードパーティクル）
        int windSpawnCount = isDiving_ ? 8 : (isIntro_ ? 6 : 2);
        std::uniform_real_distribution<float> distOffsetX(-7.0f, 7.0f);
        std::uniform_real_distribution<float> distOffsetY(-2.5f, 3.5f);
        std::uniform_real_distribution<float> distOffsetZ(10.0f, 35.0f);

        for (int i = 0; i < windSpawnCount; ++i)
        {
            Vector3 windSpawnPos = {
                curPos.x + distOffsetX(randomEngine_),
                curPos.y + distOffsetY(randomEngine_),
                curPos.z + distOffsetZ(randomEngine_)
            };
            windEffect_.SetPosition(windSpawnPos);
            windEffect_.Play();
        }
        windEffect_.Update(dt, viewMatrix, projectionMatrix, billboardMatrix);
    }

#ifdef ENABLE_EDITOR
    // フレーム描画スコープ外（NewFrame前）の場合はImGui描画をスキップ
    if (!ImGui::GetCurrentContext() || ImGui::GetFrameCount() <= 0)
    {
        return;
    }

    // エディターモードが無効（全画面表示時）はエディタUIを描画しない
    if (!services->GetEditorMode())
    {
        return;
    }

    // =========================================================================
    // 統合エディタUI: [左] メニュー  [右] インスペクター  [下] タイムライン
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
            if (!isDiving_)
            {
                isDiving_ = true;
                diveTimer_ = 0.0f;
                diveStartPos_ = playerPos_;
                diveStartRot_ = playerRot_;
                auto sceneManager = GetSceneManager();
                if (sceneManager && !sceneManager->IsTransitioning())
                {
                    sceneManager->ChangeScene("GAMEPLAY", 1.0f);
                }
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
                    if (!isDiving_)
                    {
                        isDiving_ = true;
                        diveTimer_ = 0.0f;
                        diveStartPos_ = playerPos_;
                        diveStartRot_ = playerRot_;
                        auto sm = GetSceneManager();
                        if (sm && !sm->IsTransitioning())
                        {
                            sm->ChangeScene("GAMEPLAY", 1.0f);
                        }
                    }
                }
            }

            ImGui::Spacing();
            if (ImGui::CollapsingHeader("3D自機・カメラ・配置設定 (Title Visual Settings)", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::Button("前進登場演出を再再生 (Replay Intro)", ImVec2(-1, 30)))
                {
                    isIntro_ = true;
                    introTimer_ = 0.0f;
                    isDiving_ = false;
                }

                if (ImGui::Button("降下演出テスト (Test Dive)", ImVec2(-1, 30)))
                {
                    isDiving_ = true;
                    diveTimer_ = 0.0f;
                    diveStartPos_ = targetPos_;
                    diveStartRot_ = playerRot_;
                }

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.6f, 1.0f), "■ 自機設定 (Target Transform)");
                ImGui::DragFloat3("目標位置 (TargetPos)", &targetPos_.x, 0.05f, -20.0f, 20.0f);
                ImGui::DragFloat3("自機回転 (Rotation)", &playerRot_.x, 0.02f, -6.28f, 6.28f);
                ImGui::DragFloat3("自機スケール", &playerScale_.x, 0.01f, 0.01f, 5.0f);
                
                float col[4] = { playerColor_.x, playerColor_.y, playerColor_.z, playerColor_.w };
                if (ImGui::ColorEdit4("自機カラー", col))
                {
                    playerColor_ = { col[0], col[1], col[2], col[3] };
                }
                ImGui::SliderFloat("環境反射係数", &envCoefficient_, 0.0f, 1.0f, "%.2f");
                ImGui::DragFloat3("スラスターノズル位置", &nozzleOffset_.x, 0.01f, -5.0f, 5.0f);

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.2f, 1.0f), "■ 降下演出設定 (Dive Settings)");
                ImGui::SliderAngle("降下時ロール角 (Z軸のみ)", &diveRollAngle_, -360.0f, 0.0f);
                ImGui::DragFloat("右大回り幅 (Arc X)", &diveArcWidthX_, 0.2f, 0.0f, 25.0f);
                ImGui::DragFloat("降下深さ (Drop Y)", &diveDropDistanceY_, 0.5f, 5.0f, 50.0f);
                ImGui::DragFloat("前進距離 (Forward Z)", &diveForwardDistanceZ_, 0.5f, 5.0f, 50.0f);
                ImGui::DragFloat("演出時間 (Duration)", &diveDuration_, 0.05f, 0.5f, 3.0f);

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "■ 浮遊（ホバリング）演出");
                ImGui::Checkbox("ホバリング有効", &isHovering_);
                if (isHovering_)
                {
                    ImGui::SliderFloat("浮遊の振幅", &hoverAmplitude_, 0.0f, 0.3f, "%.3f");
                    ImGui::SliderFloat("浮遊の周期速度", &hoverSpeed_, 0.1f, 5.0f, "%.2f");
                }

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "■ カメラ (Title Camera)");
                ImGui::DragFloat3("カメラ位置", &cameraPos_.x, 0.05f, -30.0f, 30.0f);
                ImGui::DragFloat3("カメラ回転", &cameraRot_.x, 0.01f, -3.14f, 3.14f);

                ImGui::Spacing();
                if (ImGui::Button("ユーザー指定値にリセット (x=0,y=0,z=-3, rot=0)"))
                {
                    targetPos_ = { 0.0f, 0.0f, -3.0f };
                    playerRot_ = { 0.0f, 0.0f, 0.0f };
                    cameraPos_ = { 0.0f, 0.7f, -8.5f };
                    cameraRot_ = { 0.05f, 0.0f, 0.0f };
                    nozzleOffset_ = { 0.0f, -0.20f, -2.15f };
                    diveRollAngle_ = -3.14159265f;
                    diveArcWidthX_ = 7.0f;
                    diveDropDistanceY_ = 22.0f;
                    diveForwardDistanceZ_ = 24.0f;
                    diveDuration_ = 1.2f;
                    isDiving_ = false;
                }

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.8f, 0.4f, 1.0f, 1.0f), "■ サウンド設定 (Audio)");
                float bgmVol = SoundManager::GetInstance()->GetBGMVolume();
                if (ImGui::SliderFloat("BGM音量", &bgmVol, 0.0f, 1.0f, "%.2f"))
                {
                    SoundManager::GetInstance()->SetBGMVolume(bgmVol);
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
#endif // ENABLE_EDITOR
}

void TitleScene::Draw()
{
    auto services = Services();
    if (!services) return;

    auto object3dCommon = services->GetObject3dCommon();

    // 1. スカイボックス描画
    if (skybox_)
    {
        skybox_->Draw();
    }

    // 2. 自機モデル描画
    if (object3dCommon)
    {
        object3dCommon->SetCommonDrawSetting();
    }

    if (playerObj_)
    {
        playerObj_->Draw();
    }

    // 3. スラスターブースト＆気流粒子パーティクル描画
    thrusterEffect_.Draw();
    windEffect_.Draw();
}

void TitleScene::Finalize()
{
    // タイトルBGM停止
    SoundManager::GetInstance()->StopBGM();

    playerObj_.reset();
    skybox_.reset();
    camera_.reset();

    BaseScene::Finalize();
}
