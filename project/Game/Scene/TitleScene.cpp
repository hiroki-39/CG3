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

    debugCamera_ = std::make_unique<Camera>();
    debugCamera_->SetTranslate(cameraPos_);
    debugCamera_->SetRotation(cameraRot_);

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
        ParticleManager::GetInstance()->RegisterCylinder("Cylinder", "resources/sprites/effect/gradationLine.png", 16, 0.5f, 0.5f, 4.0f);

        thrusterEffect_.Initialize(dxCommon, srvManager);
        thrusterEffect_.LoadFromJson("thruster.json");
        if (playerObj_)
        {
            thrusterEffect_.SetParentMatrix(&playerObj_->GetmatWorld());
        }
        thrusterEffect_.SetPosition(nozzleOffset_);

        windEffect_.Initialize(dxCommon, srvManager);
        windEffect_.LoadFromJson("wind.json");
        for (auto& node : windEffect_.GetNodes())
        {
            node->emitter.SetUseBillboard(false);
        }

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

void TitleScene::UpdateDebugCamera(float dt)
{
    auto services = Services();
    if (!services || !debugCamera_) return;

    auto input = services->GetInput();
    if (!input) return;

    const float kRotateSpeed = 0.005f;
    const float kMoveSpeed = debugCameraMoveSpeed_;

    LONG dx = input->GetMouseMoveX();
    LONG dy = input->GetMouseMoveY();
    LONG wheel = input->GetMouseWheel();

    // マウス右ボタンドラッグでカメラ視点回転
    if (input->PushMouseButton(1))
    {
        Vector3 rot = debugCamera_->GetRotation();
        rot.y += static_cast<float>(dx) * kRotateSpeed;
        rot.x += static_cast<float>(dy) * kRotateSpeed;

        const float kMaxPitch = 1.5f;
        const float kMinPitch = -1.5f;
        rot.x = std::clamp(rot.x, kMinPitch, kMaxPitch);

        debugCamera_->SetRotation(rot);
    }

    // 移動速度（Shiftキーで加速）
    float currentMoveSpeed = kMoveSpeed;
    if (input->PushKey(DIK_LSHIFT) || input->PushKey(DIK_RSHIFT))
    {
        currentMoveSpeed *= 3.0f;
    }
    float moveStep = currentMoveSpeed * dt;

    Vector3 pos = debugCamera_->GetTranslate();
    Vector3 rot = debugCamera_->GetRotation();
    float yaw = rot.y;

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

    // WASD による水平移動
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

    // E / Q による上下移動
    if (input->PushKey(DIK_E))
    {
        pos.y += moveStep;
    }
    if (input->PushKey(DIK_Q))
    {
        pos.y -= moveStep;
    }

    // マウスホイールによる前後移動
    if (wheel != 0)
    {
        float wheelStep = static_cast<float>(wheel) * 0.01f;
        pos.x += forward.x * wheelStep;
        pos.z += forward.z * wheelStep;
    }

    debugCamera_->SetTranslate(pos);
    debugCamera_->Update();
}

void TitleScene::Update()
{
    UpdateSprites();

    auto services = Services();
    if (!services) return;

    auto input = services->GetInput();
    auto object3dCommon = services->GetObject3dCommon();

#ifdef ENABLE_EDITOR
    bool isPlaying = services->IsGamePlaying();
#else
    bool isPlaying = true;
#endif
    const float dt = isPlaying ? (1.0f / 60.0f) : 0.0f;

    // -------------------------------------------------------------
    // デバッグカメラ切り替え（F2キー）
    // -------------------------------------------------------------
    if (input && input->TriggerKey(DIK_F2))
    {
        isDebugCamera_ = !isDebugCamera_;
        if (isDebugCamera_ && camera_ && debugCamera_)
        {
            debugCamera_->SetTranslate(camera_->GetTranslate());
            debugCamera_->SetRotation(camera_->GetRotation());
            debugCamera_->Update();
        }
    }

    // -------------------------------------------------------------
    // 1. カメラ更新とアクティブカメラの決定（自機・描画オブジェクトより先に確定）
    // -------------------------------------------------------------
    if (camera_)
    {
        camera_->SetTranslate(cameraPos_);
        camera_->SetRotation(cameraRot_);
        camera_->Update();
    }

    if (isDebugCamera_)
    {
        // デバッグカメラは一時停止中（Pause）でも動かせるように固定デルタタイムを使用
        UpdateDebugCamera(1.0f / 60.0f);
    }

    // デバッグカメラ有効時はデバッグカメラをアクティブカメラとして使用
    Camera* activeCamera = (isDebugCamera_ && debugCamera_) ? debugCamera_.get() : camera_.get();

    // 描画エンジンにアクティブカメラを設定
    if (object3dCommon && activeCamera)
    {
        object3dCommon->SetDefaultCamera(activeCamera);
    }

    // -------------------------------------------------------------
    // 決定ボタン押下時の判定（自機を傾けて急降下しゲーム開始）
    // -------------------------------------------------------------
    bool isDecisionPressed = false;

    if (input && isPlaying)
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
    // 2. 自機のアニメーション計算・更新
    // -------------------------------------------------------------
    if (isPlaying)
    {
        idleTimer_ += dt;
    }

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
        // === 定位置での巡航・リアル飛行演出（大気中の飛行感覚・左右上下のゆったりとした移動とバンク） ===
        curPos = targetPos_;
        curRot = playerRot_;

        if (isFlightMotion_)
        {
            // 左右のゆったりとしたS字旋回・ドリフト移動
            float tFlight = idleTimer_ * flightSpeed_;
            float driftX = std::sinf(tFlight * 0.7f) * flightDriftX_ + std::sinf(tFlight * 0.35f) * (flightDriftX_ * 0.35f);

            // 上下の自然な高度調整（2つの波の合成で有機的な浮遊感）
            float driftY = std::sinf(tFlight * 1.3f) * flightDriftY_ + std::cosf(tFlight * 0.65f) * (flightDriftY_ * 0.4f);

            // 前後のわずかなピッチング感
            float driftZ = std::cosf(tFlight * 0.85f) * 0.08f;

            curPos.x += driftX;
            curPos.y += driftY;
            curPos.z += driftZ;

            // 姿勢の連動（左右移動に伴う自然なバンク角、昇降に伴うピッチ角）
            // 左右移動速度（Xの微分）に応じてロール（Z軸）が傾く
            float rollVelocity = (std::cosf(tFlight * 0.7f) * 0.7f * flightDriftX_ + std::cosf(tFlight * 0.35f) * 0.35f * (flightDriftX_ * 0.35f));
            float rollAngle = rollVelocity * -flightBankAmount_;

            // 上下昇降速度に応じて機首（ピッチ X軸）がわずかに頷く
            float pitchVelocity = (std::cosf(tFlight * 1.3f) * 1.3f * flightDriftY_);
            float pitchAngle = pitchVelocity * -0.04f;

            // ロールに伴うわずかなヨー（Y軸）旋回
            float yawAngle = rollAngle * 0.25f;

            curRot.x += pitchAngle;
            curRot.y += yawAngle;
            curRot.z += rollAngle;
        }

        cameraRot_.x = 0.05f;
    }

    playerPos_ = curPos;

    // 自機モデルの更新（現在のアクティブカメラを直接適用して更新）
    if (playerObj_)
    {
        playerObj_->SetCamera(activeCamera);
        playerObj_->SetTranslate(curPos);
        playerObj_->SetRotation(curRot);
        playerObj_->SetScale(playerScale_);
        playerObj_->SetColor(playerColor_);
        playerObj_->SetEnvironmentCoefficient(envCoefficient_);
        playerObj_->Update();
    }

    // -------------------------------------------------------------
    // 3. スカイボックス更新
    // -------------------------------------------------------------
    if (skybox_ && activeCamera)
    {
        skybox_->SetCamera(activeCamera);
        skybox_->Update();
    }

    // -------------------------------------------------------------
    // ブースト（スラスター）＆周辺粒子（気流線）の更新
    // -------------------------------------------------------------
    if (activeCamera)
    {
        Matrix4x4 viewMatrix = activeCamera->GetViewMatrix();
        Matrix4x4 projectionMatrix = activeCamera->GetProjectionMatrix();
        Matrix4x4 billboardMatrix = Billboard::CreateFromCamera(activeCamera, true);

        // 1. スラスター（ブースト炎）
        // 自機（playerObj_）と親子関係を結び、機体の姿勢・移動・ロール回転に完全連動
        if (playerObj_)
        {
            thrusterEffect_.SetParentMatrix(&playerObj_->GetmatWorld());
        }
        thrusterEffect_.SetPosition(nozzleOffset_);

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
        if (isPlaying)
        {
            thrusterEffect_.Play();
        }
        thrusterEffect_.Update(dt, viewMatrix, projectionMatrix, billboardMatrix);

        // 2. スピードライン（奥から手前へ一直線に突き抜ける光の筋）
        if (isPlaying)
        {
            int speedLineSpawnCount = isDiving_ ? 8 : (isIntro_ ? 6 : 3);
            std::uniform_real_distribution<float> distOffsetX(-10.0f, 10.0f);
            std::uniform_real_distribution<float> distOffsetY(-3.5f, 4.5f);
            std::uniform_real_distribution<float> distOffsetZ(18.0f, 42.0f);

            for (int i = 0; i < speedLineSpawnCount; ++i)
            {
                float ox = distOffsetX(randomEngine_);
                if (std::abs(ox) < 1.4f) { ox = (ox >= 0.0f ? 1.4f : -1.4f); }
                float oy = distOffsetY(randomEngine_);

                Vector3 speedLineSpawnPos = {
                    curPos.x + ox,
                    curPos.y + oy,
                    curPos.z + distOffsetZ(randomEngine_)
                };

                windEffect_.SetPosition(speedLineSpawnPos);
                windEffect_.Play();
            }
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
        if (services->IsGamePlaying())
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.55f, 0.15f, 1.0f));
            if (ImGui::Button("一時停止 (Pause)", ImVec2(-1, 30)))
            {
                services->SetGamePlaying(false);
            }
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.68f, 0.30f, 1.0f));
            if (ImGui::Button("ゲーム再生 (Play)", ImVec2(-1, 30)))
            {
                services->SetGamePlaying(true);
            }
            ImGui::PopStyleColor();
        }

        if (ImGui::Button("ゲームプレイ開始 (SPACE)", ImVec2(-1, 32)))
        {
            if (!isDiving_)
            {
                services->SetGamePlaying(true);
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
            if (ImGui::CollapsingHeader("ゲームプレイ制御 (Playback Control)", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (services->IsGamePlaying())
                {
                    if (ImGui::Button("一時停止 (Pause)", ImVec2(130, 36)))
                    {
                        services->SetGamePlaying(false);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("全画面 (F1で復帰)", ImVec2(140, 36)))
                    {
                        services->SetEditorMode(false);
                        services->SetGamePlaying(true);
                    }
                    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "状態: プレイ中 (PLAYING)");
                }
                else
                {
                    if (ImGui::Button("開始 (Play)", ImVec2(130, 36)))
                    {
                        services->SetGamePlaying(true);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("全画面プレイ", ImVec2(140, 36)))
                    {
                        services->SetGamePlaying(true);
                        services->SetEditorMode(false);
                    }
                    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "状態: 停止中 (STOPPED - BGM/SE待機)");
                    ImGui::TextDisabled("※再生ボタンを押すまでBGMとSEは鳴りません");
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
                        services->SetGamePlaying(true);
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
                    thrusterEffect_.ClearParticles();
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
                ImGui::TextColored(ImVec4(0.2f, 0.9f, 1.0f, 1.0f), "■ 巡航飛行演出 (Flight Motion)");
                ImGui::Checkbox("巡航飛行アニメーション有効", &isFlightMotion_);
                if (isFlightMotion_)
                {
                    ImGui::SliderFloat("左右ドリフト幅 (Drift X)", &flightDriftX_, 0.0f, 2.0f, "%.2f");
                    ImGui::SliderFloat("上下ドリフト幅 (Drift Y)", &flightDriftY_, 0.0f, 1.0f, "%.2f");
                    ImGui::SliderFloat("巡航周期速度 (Speed)", &flightSpeed_, 0.1f, 3.0f, "%.2f");
                    ImGui::SliderFloat("旋回バンク傾き量 (Bank)", &flightBankAmount_, 0.0f, 0.2f, "%.3f");
                }

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "■ スピードライン演出 (Speed Line)");
                auto& speedNodes = windEffect_.GetNodes();
                if (!speedNodes.empty())
                {
                    auto& emitter = speedNodes[0]->emitter;
                    auto param = emitter.GetParameter();
                    float currentSpeed = -param.minVelocity.z;
                    if (ImGui::SliderFloat("ライン速度 (Speed)", &currentSpeed, 10.0f, 200.0f, "%.1f"))
                    {
                        param.minVelocity.z = -currentSpeed;
                        param.maxVelocity.z = -(currentSpeed * 1.4f);
                        emitter.SetParameter(param);
                    }
                    if (ImGui::SliderFloat("生存時間 (LifeTime)", &param.maxLifeTime, 0.3f, 2.5f, "%.2f"))
                    {
                        param.minLifeTime = param.maxLifeTime * 0.65f;
                        emitter.SetParameter(param);
                    }
                    if (ImGui::SliderFloat("ライン長さ (Length)", &param.maxScale.y, 0.5f, 5.0f, "%.2f"))
                    {
                        param.minScale.y = param.maxScale.y * 0.45f;
                        emitter.SetParameter(param);
                    }
                }

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "■ カメラ (Title Camera)");
                ImGui::DragFloat3("通常カメラ位置", &cameraPos_.x, 0.05f, -30.0f, 30.0f);
                ImGui::DragFloat3("通常カメラ回転", &cameraRot_.x, 0.01f, -3.14f, 3.14f);

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.7f, 1.0f), "■ デバッグカメラ (Debug Camera)");
                if (ImGui::Checkbox("デバッグカメラ有効 (F2)", &isDebugCamera_))
                {
                    if (isDebugCamera_ && camera_ && debugCamera_)
                    {
                        debugCamera_->SetTranslate(camera_->GetTranslate());
                        debugCamera_->SetRotation(camera_->GetRotation());
                        debugCamera_->Update();
                    }
                }
                if (isDebugCamera_ && debugCamera_)
                {
                    Vector3 dbgPos = debugCamera_->GetTranslate();
                    Vector3 dbgRot = debugCamera_->GetRotation();
                    if (ImGui::DragFloat3("デバッグカメラ位置", &dbgPos.x, 0.05f, -50.0f, 50.0f))
                    {
                        debugCamera_->SetTranslate(dbgPos);
                    }
                    if (ImGui::DragFloat3("デバッグカメラ回転", &dbgRot.x, 0.01f, -3.14f, 3.14f))
                    {
                        debugCamera_->SetRotation(dbgRot);
                    }
                    ImGui::SliderFloat("カメラ移動速度", &debugCameraMoveSpeed_, 1.0f, 50.0f, "%.1f");
                    if (ImGui::Button("通常カメラの位置・回転にリセット"))
                    {
                        if (camera_)
                        {
                            debugCamera_->SetTranslate(camera_->GetTranslate());
                            debugCamera_->SetRotation(camera_->GetRotation());
                        }
                    }
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "操作方法:");
                    ImGui::BulletText("右ドラッグ: 視点回転");
                    ImGui::BulletText("WASD: 前後左右移動 (Shiftで加速)");
                    ImGui::BulletText("E / Q: 上昇 / 下降");
                    ImGui::BulletText("マウスホイール: 前後移動");
                }

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

    // 3. スラスターブースト＆スピードラインパーティクル描画
    thrusterEffect_.Draw();
    windEffect_.Draw();
}

void TitleScene::Finalize()
{
    // タイトルBGM停止
    SoundManager::GetInstance()->StopBGM();

    playerObj_.reset();
    skybox_.reset();
    debugCamera_.reset();
    camera_.reset();

    BaseScene::Finalize();
}
