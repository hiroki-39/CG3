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
    if (object3dCommon)
    {
        object3dCommon->SetDefaultCamera(camera.get());
    }
    
    camera->SetTranslate({ 0.0f, 6.0f, -20.0f });
    camera->SetRotation({ 0.3f, 0.0f, 0.0f });

    // アセット登録
    ParticleManager::GetInstance()->RegisterQuad("quad", "resources/circle.png");

    uint32_t instancingSrvIndex = UINT32_MAX;
    currentBlendModeIndex = static_cast<int>(BlendMode::Additive);

    auto texManager = TextureManager::GetInstance();
    dxCommon->BeginTextureUploadBatch();

    // スカイボックスの初期化
    skybox_ = std::make_unique<Skybox>();
    // DDSキューブマップファイルのパスを指定してください
    skybox_->Initialize(dxCommon, "resources/rostock_laage_airport_4k.dds");

    // モデル読み込み
    ModelManager::GetInstance()->LoadModel("plane.obj");
    ModelManager::GetInstance()->LoadModel("Cube.obj");
    ModelManager::GetInstance()->LoadModel("monsterBall.obj");
    ModelManager::GetInstance()->LoadModel("terrain.obj");

    // スプライト用テクスチャ読み込み
    texManager->LoadTexture("uvChecker.png");
    texManager->LoadTexture("monsterBall.png");
    texManager->LoadTexture("checkerBoard.png");
    texManager->LoadTexture("circle.png");
    texManager->LoadTexture("white.png");

    texManager->ExecuteUploadCommands();

    uint32_t uvCheckerTex = TextureManager::GetInstance()->GetTextureIndexByFilePath("uvChecker.png");
    uint32_t monsterBallTex = TextureManager::GetInstance()->GetTextureIndexByFilePath("monsterBall.png");
    uint32_t checkerBoardTex = TextureManager::GetInstance()->GetTextureIndexByFilePath("checkerBoard.png");

    particleSrvIndex = TextureManager::GetInstance()->GetSrvIndex("circle.png");

    ParticleManager::GetInstance()->SetupRendererFromAsset(particleRenderer, "quad", dxCommon, srvManager, kNumMaxInstance);
    instancingData = particleRenderer.GetInstancingData();
    instancingSrvIndex = particleRenderer.GetInstancingSrvIndex();

    texManager->ClearIntermediateResources();

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
        obj->SetModel("cube.obj");
        obj->SetTranslate(Vector3(0.0f, 1.0f, -4.0f));
        obj->SetRotation(Vector3(0.0f, 0.0f, 0.0f));
        obj->SetScale(Vector3(1.0f, 1.0f, 1.0f));
        modelInstances.push_back(std::move(obj));

        auto terrain = std::make_unique<Object3d>();
        terrain->Initialize(object3dCommon);
        terrain->SetModel("terrain.obj");
        terrain->SetTranslate(Vector3(0.0f, 0.0f, 0.0f));
        terrain->SetRotation(Vector3(0.0f, 0.0f, 0.0f));
        terrain->SetScale(Vector3(1.0f, 1.0f, 1.0f));
        modelInstances.push_back(std::move(terrain));
    }

    Data = SoundManager::GetInstance()->SoundLoadFile("bgm.mp3");

    // ParticleSystem 初期設定
    particleSystem.GetEmitter().GetEmitter().count = 3;
    particleSystem.GetEmitter().GetEmitter().frequency = 0.5f;
    particleSystem.GetEmitter().GetEmitter().frequencyTime = 0.0f;
    particleSystem.GetEmitter().GetEmitter().transform.translate = { 0.0f,-2.0f,0.0f };
    particleSystem.GetEmitter().GetEmitter().transform.rotation = { 0.0f,0.0f,0.0f };
    particleSystem.GetEmitter().GetEmitter().transform.scale = { 1.0f,1.0f,1.0f };

    currentEffect = ParticleEffect::Wind;
    particleSystem.SetEffect(currentEffect);
    particleSystem.AddInitialParticles(randomEngine, kNumMaxInstance);

    AccelerationField accelerationField;
    accelerationField.accleration = { -15.0f, 0.0f, 0.0f };
    accelerationField.area.min = { -1.0f, -1.0f, -1.0f };
    accelerationField.area.max = { 1.0f, 1.0f, 1.0f };
    particleSystem.SetAccelerationField(accelerationField);
}

void GamePlayScene::Finalize()
{
    // unique_ptr 管理なので明示的な delete は不要
    sprites.clear();
    modelInstances.clear();

    sound.Stop();
    SoundManager::GetInstance()->SoundUnload(&Data);

    skybox_.reset();
    camera.reset();
}

void GamePlayScene::Update()
{
    auto services = EngineServices::GetInstance();
    auto input = services->GetInput();

    // --- カメラ操作 ---
    // ・ホイール押し込み（ミドルボタン）を押しながら移動 -> カメラ回転（yaw/pitch）(継続)
    // ・WASDキー -> カメラ移動（カメラの向きに沿った前後左右）
    // ・ホイール回転 -> ズーム (継続)
    if (input && camera)
    {
        // 感度設定（必要に応じて調整）
        const float kRotateSpeed = 0.005f; // 回転感度（ラジアン換算想定）
        const float kMoveSpeed = 8.0f;     // 移動速度（ワールド単位 / 秒）
        const float kZoomSpeed = 0.0015f;  // ホイール感度（調整可）

        LONG dx = input->GetMouseMoveX();
        LONG dy = input->GetMouseMoveY();
        LONG wheel = input->GetMouseWheel();

        // ミドルボタン（ホイール押し込み）で回転（既存の挙動を維持）
        if (input->PushMouseButton(2))
        {
            Vector3 rot = camera->GetRotation();
            // マウス右移動で yaw 増加、下移動で pitch 増加（上下反転は好みで調整）
            rot.y += static_cast<float>(dx) * kRotateSpeed;
            rot.x += static_cast<float>(dy) * kRotateSpeed;

            // ピッチ（X軸回転）を適度に制限（直上直下で反転しないように）
            const float kMaxPitch = 1.5f;  // 約 85度
            const float kMinPitch = -1.5f; // 約 -85度
            rot.x = std::clamp(rot.x, kMinPitch, kMaxPitch);

            camera->SetRotation(rot);
        }
        else
        {
            // WASDキーでカメラ移動（カメラのyawに沿った前後左右）
            float moveStep = kMoveSpeed * kDeltaTime_;
            Vector3 pos = camera->GetTranslate();
            Vector3 rot = camera->GetRotation();
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

            camera->SetTranslate(pos);
        }

        // ホイールでズーム（既存の挙動）
        if (wheel != 0)
        {
            Vector3 pos = camera->GetTranslate();
            // wheel は通常 ±120（1ノッチ）を返す。符号は環境で調整してください。
            pos.z += static_cast<float>(-wheel) * kZoomSpeed;
            camera->SetTranslate(pos);
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
    if (camera) camera->Update();
    for (auto& model : modelInstances) if (model) model->Update();
    for (auto& sprite : sprites) if (sprite) sprite->Update();

    if (input && input->TriggerKey(DIK_SPACE))
    {
        sound.SoundPlayWave(SoundManager::GetInstance()->GetXAudio2(), Data);
    }

    // カメラ行列の取得
    Matrix4x4 cameraMatrix = camera->GetWorldMatrix();
    Matrix4x4 viewMatrix = camera->GetViewMatrix();
    Matrix4x4 projectionMatrix = camera->GetProjectionMatrix();

    // ビルボード行列（Camera* を渡す）
    Matrix4x4 billboardMatrix = Billboard::CreateFromCamera(camera.get(), useBillboard);


    if (update)
    {
        particleSystem.Update(kDeltaTime_);
    }

    numInstance = particleSystem.FillInstancingBuffer(instancingData, kNumMaxInstance, viewMatrix, projectionMatrix, billboardMatrix, false);

#ifdef USE_IMGUI

    // --- Sprite ウィンドウ ---
    ImGui::Begin("Sprite");
    if (!sprites.empty())
    {
        // unique_ptr から生ポインタを取得
        Sprite* s = sprites[0].get();

        bool display = isDisplaySprite;
        if (ImGui::Checkbox("Display Sprite", &display))
        {
            isDisplaySprite = display;
        }

        Vector2 pos = s->GetPosition();
        float posArr[2] = { pos.x, pos.y };
        if (ImGui::DragFloat2("Position", posArr, 1.0f))
        {
            s->SetPosition(Vector2(posArr[0], posArr[1]));
        }

        Vector2 size = s->GetSize();
        float sizeArr[2] = { size.x, size.y };
        if (ImGui::DragFloat2("Size", sizeArr, 1.0f, 1.0f, 4096.0f))
        {
            s->SetSize(Vector2(sizeArr[0], sizeArr[1]));
        }

        float rotation = s->GetRotation();
        if (ImGui::DragFloat("Rotation", &rotation, 0.5f))
        {
            s->SetRotation(rotation);
        }

        Vector4 col = s->GetColor();
        float colArr[4] = { col.x, col.y, col.z, col.w };
        if (ImGui::ColorEdit4("Color", colArr))
        {
            s->SetColor(Vector4(colArr[0], colArr[1], colArr[2], colArr[3]));
        }
    }
    ImGui::End();


    // --- Model ウィンドウ ---
    ImGui::Begin("Model");
    if (!modelInstances.empty())
    {
        Object3d* obj = modelInstances[0].get();
        // Translate / Rotation / Scale
        Vector3 t = obj->GetTranslate();
        float tArr[3] = { t.x, t.y, t.z };
        if (ImGui::DragFloat3("Translate", tArr, 0.05f))
        {
            obj->SetTranslate(Vector3(tArr[0], tArr[1], tArr[2]));
        }

        Vector3 r = obj->GetRotation();
        float rArr[3] = { r.x, r.y, r.z };
        if (ImGui::DragFloat3("Rotation", rArr, 0.5f))
        {
            obj->SetRotation(Vector3(rArr[0], rArr[1], rArr[2]));
        }

        Vector3 s = obj->GetScale();
        float sArr[3] = { s.x, s.y, s.z };
        if (ImGui::DragFloat3("Scale", sArr, 0.01f, 0.001f, 100.0f))
        {
            obj->SetScale(Vector3(sArr[0], sArr[1], sArr[2]));
        }
    }

    if (!modelInstances.empty())
    {
        // 最初のインスタンスの Model を参照して現在値を取得
        Model* sampleModel = modelInstances[0]->GetModel();
        if (sampleModel)
        {
            int currentSelect = sampleModel->GetSelectLightings();

            // ラベルは HLSL の case に対応させる（0..5）
            const char* lightingNames[] = {
                "0: TextureOnly",
                "1: Directional (Diffuse)",
                "2: Directional (Soft)",
                "3: Directional (Diffuse+Specular)",
                "4: Directional + Point",
                "5: Spot"
            };

            // Combo で選択（HLSL の switch の case に対応）
            if (ImGui::Combo("Select Lighting Mode", &currentSelect, lightingNames, IM_ARRAYSIZE(lightingNames)))
            {
                // 全インスタンスに反映
                for (auto& obj : modelInstances)
                {
                    if (!obj) continue;
                    Model* m = obj->GetModel();
                    if (m) m->SetSelectLightings(currentSelect);
                }
            }
        }
    }

    ImGui::End();


    // --- Camera ウィンドウ (分離) ---
    ImGui::Begin("Camera");
    if (camera)
    {
        Vector3& camPosRef = camera->GetTranslate();
        float camPosArr[3] = { camPosRef.x, camPosRef.y, camPosRef.z };
        if (ImGui::DragFloat3("Translate", camPosArr, 0.1f))
        {
            camera->SetTranslate(Vector3(camPosArr[0], camPosArr[1], camPosArr[2]));
        }

        Vector3& camRotRef = camera->GetRotation();
        float camRotArr[3] = { camRotRef.x, camRotRef.y, camRotRef.z };
        if (ImGui::DragFloat3("Rotation", camRotArr, 0.1f))
        {
            camera->SetRotation(Vector3(camRotArr[0], camRotArr[1], camRotArr[2]));
        }

        float fov = camera->GetFovY();
        if (ImGui::DragFloat("FOV Y", &fov, 0.01f, 0.01f, 3.14f))
        {
            camera->SetFovY(fov);
        }

        float aspect = camera->GetAspectRatio();
        if (ImGui::DragFloat("Aspect", &aspect, 0.01f, 0.1f, 10.0f))
        {
            camera->SetAspectRatio(aspect);
        }

        float nearC = camera->GetNearClip();
        if (ImGui::DragFloat("Near Clip", &nearC, 0.001f, 0.001f, 100.0f))
        {
            camera->SetNearClip(nearC);
        }

        float farC = camera->GetFarClip();
        if (ImGui::DragFloat("Far Clip", &farC, 0.1f, 1.0f, 1000.0f))
        {
            camera->SetFarClip(farC);
        }
    }
    ImGui::End();


    // --- Light ウィンドウ (モデルが使っているライトのみ表示) ---
    ImGui::Begin("Light");
    if (!modelInstances.empty())
    {
        Object3d* firstObj = modelInstances[0].get();
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
            ImGui::Text("Directional Light");

            static bool dirInit = false;
            static Vector4 dirColor = { 1.0f, 1.0f, 1.0f, 1.0f };
            static Vector3 dirDirection = { 0.0f, -1.0f, 0.0f };
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
            if (ImGui::ColorEdit4("Color##Dir", dcArr))
            {
                dirColor = Vector4(dcArr[0], dcArr[1], dcArr[2], dcArr[3]);
                for (auto& m : modelInstances) if (m) m->SetDirectionalLightColor(dirColor);
            }

            float ddArr[3] = { dirDirection.x, dirDirection.y, dirDirection.z };
            if (ImGui::DragFloat3("Direction##Dir", ddArr, 0.01f, -10.0f, 10.0f))
            {
                dirDirection = Vector3(ddArr[0], ddArr[1], ddArr[2]);
                for (auto& m : modelInstances) if (m) m->SetDirectionalLightDirection(dirDirection);
            }

            if (ImGui::DragFloat("Intensity##Dir", &dirIntensity, 0.01f, 0.0f, 100.0f))
            {
                if (dirEnabledGlobal)
                {
                    for (auto& m : modelInstances) if (m) m->SetDirectionalLightIntensity(dirIntensity);
                    dirPrevIntensityGlobal = dirIntensity;
                }
                else
                {
                    dirPrevIntensityGlobal = dirIntensity;
                }
            }

            if (ImGui::Checkbox("Enable Directional Light (global)", &dirEnabledGlobal))
            {
                if (!dirEnabledGlobal)
                {
                    for (auto& m : modelInstances) if (m) m->SetDirectionalLightIntensity(0.0f);
                }
                else
                {
                    for (auto& m : modelInstances) if (m) m->SetDirectionalLightIntensity(dirPrevIntensityGlobal);
                }
            }
        }

        // Point
        if (usesPoint(lightingMode))
        {
            ImGui::Separator();
            ImGui::Text("Point Light");

            static bool pointInit = false;
            static Vector4 pointColor = { 1.0f, 1.0f, 1.0f, 1.0f };
            static Vector3 pointPosition = { 0.0f, 5.0f, 0.0f };
            static float pointIntensity = 1.0f;
            static float prevPointIntensity = 1.0f;
            static float pointRadius = 1.0f;
            static float pointRange = 10.0f;
            static bool pointLightEnabled = true;

            if (!pointInit && firstObj)
            {
                pointColor = firstObj->GetPointLightColor();
                pointPosition = firstObj->GetPointLightPosition();
                pointIntensity = firstObj->GetPointLightIntensity();
                prevPointIntensity = pointIntensity;
                pointInit = true;
            }

            if (ImGui::Checkbox("Enable Point Light", &pointLightEnabled))
            {
                if (!pointLightEnabled)
                {
                    for (auto& m : modelInstances) if (m) m->SetPointLightIntensity(0.0f);
                }
                else
                {
                    for (auto& m : modelInstances) if (m) m->SetPointLightIntensity(prevPointIntensity);
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
            if (ImGui::ColorEdit4("Color##Point", pcArr))
            {
                pointColor = Vector4(pcArr[0], pcArr[1], pcArr[2], pcArr[3]);
                for (auto& m : modelInstances) if (m) m->SetPointLightColor(pointColor);
            }

            float ppArr[3] = { pointPosition.x, pointPosition.y, pointPosition.z };
            if (ImGui::DragFloat3("Position##Point", ppArr, 0.05f, -100.0f, 100.0f))
            {
                pointPosition = Vector3(ppArr[0], ppArr[1], ppArr[2]);
                for (auto& m : modelInstances) if (m) m->SetPointLightPosition(pointPosition);
            }

            if (ImGui::DragFloat("Intensity##Point", &pointIntensity, 0.01f, 0.0f, 100.0f))
            {
                if (pointLightEnabled)
                {
                    for (auto& m : modelInstances) if (m) m->SetPointLightIntensity(pointIntensity);
                    prevPointIntensity = pointIntensity;
                }
                else
                {
                    prevPointIntensity = pointIntensity;
                }
            }

            if (ImGui::DragFloat("Radius##Point", &pointRadius, 0.01f, 0.1f, 100.0f))
            {
                for (auto& m : modelInstances) if (m) m->SetPointLightRadius(pointRadius);
            }
            if (ImGui::DragFloat("Range(Decay)##Point", &pointRange, 0.01f, 0.1f, 50.0f))
            {
                for (auto& m : modelInstances) if (m) m->SetPointLightDecry(pointRange);
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
            ImGui::Text("Spot Light");

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

            if (ImGui::Checkbox("Enable Spot Light", &spotLightEnabled))
            {
                if (!spotLightEnabled)
                {
                    for (auto& m : modelInstances) if (m) m->SetSpotLightIntensity(0.0f);
                }
                else
                {
                    for (auto& m : modelInstances) if (m) m->SetSpotLightIntensity(prevSpotIntensity);
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
                if (ImGui::ColorEdit4("Color##Spot", scArr))
                {
                    spotColor = Vector4(scArr[0], scArr[1], scArr[2], scArr[3]);
                    for (auto& m : modelInstances) if (m) m->SetSpotLightColor(spotColor);
                }
            }

            // Position
            {
                float spArr[3] = { spotPosition.x, spotPosition.y, spotPosition.z };
                if (ImGui::DragFloat3("Position##Spot", spArr, 0.05f, -100.0f, 100.0f))
                {
                    spotPosition = Vector3(spArr[0], spArr[1], spArr[2]);
                    for (auto& m : modelInstances) if (m) m->SetSpotLightPosition(spotPosition);
                }
            }

            // Direction
            {
                float sdArr[3] = { spotDirection.x, spotDirection.y, spotDirection.z };
                if (ImGui::DragFloat3("Direction##Spot", sdArr, 0.01f, -10.0f, 10.0f))
                {
                    spotDirection = Vector3(sdArr[0], sdArr[1], sdArr[2]);
                    for (auto& m : modelInstances) if (m) m->SetSpotLightDirection(spotDirection);
                }
            }

            // Intensity
            if (ImGui::DragFloat("Intensity##Spot", &spotIntensity, 0.01f, 0.0f, 100.0f))
            {
                if (spotLightEnabled)
                {
                    for (auto& m : modelInstances) if (m) m->SetSpotLightIntensity(spotIntensity);
                    prevSpotIntensity = spotIntensity;
                }
                else
                {
                    prevSpotIntensity = spotIntensity;
                }
            }

            // Distance / Decay
            if (ImGui::DragFloat("Distance##Spot", &spotDistance, 0.1f, 0.0f, 10000.0f))
            {
                for (auto& m : modelInstances) if (m) m->SetSpotLightDistance(spotDistance);
            }
            if (ImGui::DragFloat("Decay##Spot", &spotDecay, 0.01f, 0.0f, 10.0f))
            {
                for (auto& m : modelInstances) if (m) m->SetSpotLightDecay(spotDecay);
            }

            // Angle (deg)
            if (ImGui::SliderFloat("Angle (deg)##Spot", &spotAngleDeg, 1.0f, 90.0f))
            {
                for (auto& m : modelInstances) if (m) m->SetSpotLightAngleDeg(spotAngleDeg);
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
    ImGui::Begin("Particle");

    // パーティクルの動作 ON/OFF
    if (ImGui::Checkbox("Update Particles", &update))
    {
        // フラグ変更のみ（Update の呼び出しは main ループ側で行っている）
    }

    // ビルボード切替
    if (ImGui::Checkbox("Use Billboard", &useBillboard))
    {
        // 反映は描画側の行列生成で行われる
    }

    // ブレンドモード選択
    {
        const char* blendNames[] = { "Alpha", "Additive", "Multiply", "PreMultiplied", "None" };
        if (ImGui::Combo("Blend Mode", &currentBlendModeIndex, blendNames, static_cast<int>(BlendMode::Count)))
        {
            // currentBlendModeIndex は Draw 呼び出し時に使われる
        }
    }

    // パーティクル種類選択（ParticleEffect enum に合わせる）
    {
        const char* effectNames[] = { "Wind", "Fire", "Snow", "Explosion", "Smoke", "Confetti" };
        // currentEffect は ParticleEffect 型だが ImGui 用に int にキャストする
        int effectIndex = static_cast<int>(currentEffect);
        if (ImGui::Combo("Effect", &effectIndex, effectNames, static_cast<int>(ParticleEffect::Count)))
        {
            currentEffect = static_cast<ParticleEffect>(effectIndex);
            particleSystem.SetEffect(currentEffect);
        }
    }

    // エミッター設定を直接編集
    {
        auto& emitter = particleSystem.GetEmitter().GetEmitter();

        int count = static_cast<int>(emitter.count);
        if (ImGui::DragInt("Emitter Count", &count, 1.0f, 0, 1000))
        {
            emitter.count = static_cast<uint32_t>(std::max(0, count));
        }

        float frequency = emitter.frequency;
        if (ImGui::DragFloat("Frequency", &frequency, 0.01f, 0.0f, 100.0f))
        {
            emitter.frequency = frequency;
        }

        // transform
        Vector3 t = emitter.transform.translate;
        float tArr[3] = { t.x, t.y, t.z };
        if (ImGui::DragFloat3("Emitter Translate", tArr, 0.05f, -1000.0f, 1000.0f))
        {
            emitter.transform.translate = Vector3(tArr[0], tArr[1], tArr[2]);
        }

        Vector3 rot = emitter.transform.rotation;
        float rArr[3] = { rot.x, rot.y, rot.z };
        if (ImGui::DragFloat3("Emitter Rotation", rArr, 0.1f, -360.0f, 360.0f))
        {
            emitter.transform.rotation = Vector3(rArr[0], rArr[1], rArr[2]);
        }

        Vector3 sc = emitter.transform.scale;
        float sArr[3] = { sc.x, sc.y, sc.z };
        if (ImGui::DragFloat3("Emitter Scale", sArr, 0.01f, 0.001f, 100.0f))
        {
            emitter.transform.scale = Vector3(sArr[0], sArr[1], sArr[2]);
        }
    }

    // 初期パーティクルの再生成
    if (ImGui::Button("Recreate Initial Particles"))
    {
        particleSystem.AddInitialParticles(randomEngine, kNumMaxInstance);
    }

    ImGui::End();

#endif // USE_IMGUI

}

void GamePlayScene::Draw()
{
    auto services = EngineServices::GetInstance();
    auto object3dCommon = services->GetObject3dCommon();
    auto spriteCommon = services->GetSpriteCommon();

    if (skybox_)
    {
        skybox_->Draw(camera.get());
    }

    if (object3dCommon) object3dCommon->SetCommonDrawSetting();

    for (auto& model : modelInstances) if (model) model->Draw();

    if (spriteCommon) spriteCommon->SetCommonDrawSetting();

    if (isDisplaySprite)
    {
        for (auto& sprite : sprites) if (sprite) sprite->Draw();
    }

    particleRenderer.Draw(numInstance, particleSrvIndex, currentBlendModeIndex);
}