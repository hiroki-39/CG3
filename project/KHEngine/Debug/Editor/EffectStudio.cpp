#include "EffectStudio.h"
#include "externals/imgui/imgui.h"
#include "KHEngine/Graphics/Resource/Texture/TextureManager.h"
#include "KHEngine/Graphics/Billboard/Billboard.h"
#include <filesystem>
#include <algorithm>
#include <cmath>
#include <numbers>

EffectStudio* EffectStudio::GetInstance()
{
    static EffectStudio instance;
    return &instance;
}

void EffectStudio::Initialize(DirectXCommon* dxCommon, SrvManager* srvManager)
{
    dxCommon_ = dxCommon;
    srvManager_ = srvManager;

    CreateRenderTargets();

    // プレビュー用カメラ初期化
    camera_.SetFovY(45.0f * (std::numbers::pi_v<float> / 180.0f));
    camera_.SetAspectRatio(static_cast<float>(kWidth_) / static_cast<float>(kHeight_));
    camera_.SetNearClip(0.1f);
    camera_.SetFarClip(1000.0f);

    // パーティクル基本プリミティブアセットの登録確認
    auto particleMgr = ParticleManager::GetInstance();
    particleMgr->RegisterQuad("quad", "circle2.png");
    particleMgr->RegisterRing("ring", "gradationLine.png", 32, 0.5f, 1.0f);
    particleMgr->RegisterCylinder("Cylinder", "resources/sprites/gradationLine.png");

    // プレビュー用エフェクト初期化
    previewEffect_.Initialize(dxCommon_, srvManager_);

    // JSON ファイルリストを取得
    RefreshFileList();

    // 既存のエフェクトがあれば最初にロード
    if (!jsonFiles_.empty())
    {
        // default_effect.json があればそれを優先
        auto it = std::find(jsonFiles_.begin(), jsonFiles_.end(), "default_effect.json");
        if (it != jsonFiles_.end())
        {
            selectedFileIndex_ = static_cast<int>(std::distance(jsonFiles_.begin(), it));
        }
        else
        {
            selectedFileIndex_ = 0;
        }
        currentLoadedFile_ = jsonFiles_[selectedFileIndex_];
        previewEffect_.LoadFromJson(currentLoadedFile_);
    }
}

void EffectStudio::CreateRenderTargets()
{
    auto device = dxCommon_->GetDevice();

    // 1. カラー用 RenderTexture 作成 (RGBA8_UNORM_SRGB)
    renderTextureResource_ = dxCommon_->CreateRenderTextureResource(
        kWidth_, kHeight_,
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        backgroundColor_
    );

    // 2. 深度ステンシル用テクスチャ作成 (D24_S8)
    depthStencilResource_ = dxCommon_->CreatDepthStencilTextureResource(kWidth_, kHeight_);

    // 3. 専用 RTV デスクリプタヒープ作成 (1個)
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.NumDescriptors = 1;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap_));
    rtvHandle_ = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    device->CreateRenderTargetView(renderTextureResource_.Get(), &rtvDesc, rtvHandle_);

    // 4. 専用 DSV デスクリプタヒープ作成 (1個)
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvDescriptorHeap_));
    dsvHandle_ = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    device->CreateDepthStencilView(depthStencilResource_.Get(), &dsvDesc, dsvHandle_);

    // 5. ImGui で表示するための SRV 作成
    srvIndex_ = srvManager_->Allocate();
    srvManager_->CreateSRVforTexture2D(srvIndex_, renderTextureResource_.Get(), DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 1);

    // ビューポートとシザー
    viewport_.Width = static_cast<float>(kWidth_);
    viewport_.Height = static_cast<float>(kHeight_);
    viewport_.TopLeftX = 0.0f;
    viewport_.TopLeftY = 0.0f;
    viewport_.MinDepth = 0.0f;
    viewport_.MaxDepth = 1.0f;

    scissorRect_.left = 0;
    scissorRect_.top = 0;
    scissorRect_.right = kWidth_;
    scissorRect_.bottom = kHeight_;
}

void EffectStudio::RefreshFileList()
{
    jsonFiles_.clear();
    std::filesystem::create_directories("resources/json/particles");

    try
    {
        for (const auto& entry : std::filesystem::directory_iterator("resources/json/particles"))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".json")
            {
                jsonFiles_.push_back(entry.path().filename().string());
            }
        }
        std::sort(jsonFiles_.begin(), jsonFiles_.end());
    }
    catch (...)
    {
    }

    // 現在ロード中のファイルがリスト内の何番目か更新
    selectedFileIndex_ = -1;
    for (size_t i = 0; i < jsonFiles_.size(); ++i)
    {
        if (jsonFiles_[i] == currentLoadedFile_)
        {
            selectedFileIndex_ = static_cast<int>(i);
            break;
        }
    }
}

void EffectStudio::Update(float dt)
{
    float scaledDt = dt * playbackSpeed_;

    // 自動ループ再生
    if (autoLoop_)
    {
        loopTimer_ += scaledDt;
        if (loopTimer_ >= loopInterval_)
        {
            loopTimer_ = 0.0f;
            previewEffect_.Play();
        }
    }

    // カメラ位置の更新 (注視点 + 球面座標)
    Vector3 camPos;
    camPos.x = cameraTarget_.x + cameraDistance_ * std::cos(cameraPitch_) * std::sin(cameraYaw_);
    camPos.y = cameraTarget_.y + cameraDistance_ * std::sin(cameraPitch_);
    camPos.z = cameraTarget_.z - cameraDistance_ * std::cos(cameraPitch_) * std::cos(cameraYaw_);

    camera_.SetTranslate(camPos);
    camera_.SetRotation({ cameraPitch_, cameraYaw_, 0.0f });
    camera_.Update();

    // プレビューエフェクト更新 (原点に配置)
    previewEffect_.SetPosition({ 0.0f, 0.0f, 0.0f });
    Matrix4x4 viewMat = camera_.GetViewMatrix();
    Matrix4x4 projMat = camera_.GetProjectionMatrix();
    Matrix4x4 billboardMat = Billboard::CreateFromCamera(&camera_, true);

    previewEffect_.Update(scaledDt, viewMat, projMat, billboardMat);
}

void EffectStudio::Render()
{
    if (!renderTextureResource_ || !dxCommon_ || !srvManager_) return;

    auto commandList = dxCommon_->GetCommandList();

    // 1. バリア遷移: 描画前 (Target へ遷移)
    if (currentResourceState_ != D3D12_RESOURCE_STATE_RENDER_TARGET)
    {
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = renderTextureResource_.Get();
        barrier.Transition.StateBefore = currentResourceState_;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &barrier);
        currentResourceState_ = D3D12_RESOURCE_STATE_RENDER_TARGET;
    }

    // 2. レンダーターゲット & 深度バッファをセット
    commandList->OMSetRenderTargets(1, &rtvHandle_, false, &dsvHandle_);

    // 3. 画面クリア
    float clearColor[4] = { backgroundColor_.x, backgroundColor_.y, backgroundColor_.z, backgroundColor_.w };
    commandList->ClearRenderTargetView(rtvHandle_, clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsvHandle_, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // 4. ビューポート & シザー矩形セット
    commandList->RSSetViewports(1, &viewport_);
    commandList->RSSetScissorRects(1, &scissorRect_);

    // 5. SRV ディスクリプタヒープをセット
    srvManager_->PreDraw();

    // 6. エフェクト描画
    previewEffect_.Draw();

    // 7. バリア遷移: 描画後 (ImGui表示用に PIXEL_SHADER_RESOURCE へ遷移)
    if (currentResourceState_ != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
    {
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = renderTextureResource_.Get();
        barrier.Transition.StateBefore = currentResourceState_;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &barrier);
        currentResourceState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }
}

void EffectStudio::HandleCameraInput(float contentWidth, float contentHeight)
{
    ImGuiIO& io = ImGui::GetIO();

    // 右ドラッグでオービット回転
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
    {
        ImVec2 delta = io.MouseDelta;
        cameraYaw_ += delta.x * 0.008f;
        cameraPitch_ += delta.y * 0.008f;
        // ピッチ角制限 (-85度 〜 +85度)
        cameraPitch_ = std::clamp(cameraPitch_, -1.48f, 1.48f);
    }

    // 中ボタンドラッグでパン移動
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
    {
        ImVec2 delta = io.MouseDelta;
        float panSpeed = cameraDistance_ * 0.002f;
        Vector3 right = { std::cos(cameraYaw_), 0.0f, std::sin(cameraYaw_) };
        Vector3 up = { 0.0f, 1.0f, 0.0f };

        cameraTarget_.x -= right.x * delta.x * panSpeed;
        cameraTarget_.z -= right.z * delta.x * panSpeed;
        cameraTarget_.y += delta.y * panSpeed;
    }

    // ホイールでズーム
    if (io.MouseWheel != 0.0f)
    {
        cameraDistance_ -= io.MouseWheel * (cameraDistance_ * 0.15f);
        cameraDistance_ = std::clamp(cameraDistance_, 0.5f, 50.0f);
    }
}

void EffectStudio::DrawViewportWindow()
{
#ifdef USE_IMGUI
    if (!showViewport_) return;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    if (ImGui::Begin("エフェクト画面", &showViewport_))
    {
        ImVec2 availSize = ImGui::GetContentRegionAvail();
        if (availSize.x > 10.0f && availSize.y > 10.0f)
        {
            // SrvManager から GPU ハンドルを取得
            D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = srvManager_->GetSRVGPUDescriptorHandle(srvIndex_);

            // アスペクト比を維持しながらフィッティング
            float renderAspect = static_cast<float>(kWidth_) / static_cast<float>(kHeight_);
            float viewAspect = availSize.x / availSize.y;
            ImVec2 imgSize = availSize;
            ImVec2 uv0 = ImVec2(0.0f, 0.0f);
            ImVec2 uv1 = ImVec2(1.0f, 1.0f);

            // 画像表示
            ImGui::Image((ImTextureID)srvHandle.ptr, imgSize, uv0, uv1);

            // 床グリッド線を描画（Image の領域にぴったりオーバーレイ）
            ImVec2 imgPos = ImGui::GetItemRectMin();
            DrawGridOverlay(imgPos, imgSize);

            // ビューポートホバー時のカメラ操作
            if (ImGui::IsItemHovered())
            {
                HandleCameraInput(availSize.x, availSize.y);
            }

            // ビューポート左上のクイックツールバー（半透明）
            ImGui::SetCursorScreenPos(ImVec2(imgPos.x + 10, imgPos.y + 10));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.10f, 0.65f));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
            if (ImGui::BeginChild("##QuickBar", ImVec2(230, 32), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
            {
                ImGui::Checkbox("床グリッド", &showGrid_);
                ImGui::SameLine();
                if (ImGui::SmallButton("視点リセット"))
                {
                    cameraTarget_ = { 0.0f, 0.0f, 0.0f };
                    cameraDistance_ = 6.0f;
                    cameraYaw_ = 0.0f;
                    cameraPitch_ = 0.35f;
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
#endif
}

void EffectStudio::DrawControlWindow()
{
#ifdef USE_IMGUI
    if (!showEditor_) return;

    if (ImGui::Begin("エフェクトエディター", &showEditor_))
    {
        // -------------------------------------------------------------
        // 1. プリセットファイル管理 (JSON Selector & Manager)
        // -------------------------------------------------------------
        ImGui::TextColored(ImVec4(0.3f, 0.85f, 1.0f, 1.0f), "■ エフェクトプリセット (JSON)");
        ImGui::Separator();

        // ドロップダウンで一覧から選択
        std::vector<const char*> filePtrs;
        for (const auto& file : jsonFiles_)
        {
            filePtrs.push_back(file.c_str());
        }

        if (ImGui::Combo("ファイル選択", &selectedFileIndex_, filePtrs.data(), static_cast<int>(filePtrs.size())))
        {
            if (selectedFileIndex_ >= 0 && selectedFileIndex_ < static_cast<int>(jsonFiles_.size()))
            {
                currentLoadedFile_ = jsonFiles_[selectedFileIndex_];
                previewEffect_.LoadFromJson(currentLoadedFile_);
                strncpy_s(saveAsFileName_, currentLoadedFile_.c_str(), sizeof(saveAsFileName_) - 1);
                statusMessage_ = "ロード完了: " + currentLoadedFile_;
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("更新##ReloadList"))
        {
            RefreshFileList();
            statusMessage_ = "ファイル一覧を更新しました。";
        }

        // ロード & 保存ボタン群
        if (ImGui::Button("読込 (Load)", ImVec2(100, 26)))
        {
            if (selectedFileIndex_ >= 0 && selectedFileIndex_ < static_cast<int>(jsonFiles_.size()))
            {
                currentLoadedFile_ = jsonFiles_[selectedFileIndex_];
                previewEffect_.LoadFromJson(currentLoadedFile_);
                statusMessage_ = "ロード完了: " + currentLoadedFile_;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("上書き保存 (Save)", ImVec2(130, 26)))
        {
            if (!currentLoadedFile_.empty())
            {
                previewEffect_.SaveToJson(currentLoadedFile_);
                RefreshFileList();
                statusMessage_ = "保存完了: " + currentLoadedFile_;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("新規作成 (New)", ImVec2(110, 26)))
        {
            // ノードを初期化して空のエフェクト作成
            previewEffect_.ClearNodes();
            previewEffect_.AddNode("Main Layer", 0); // デフォルトの爆発Quad
            currentLoadedFile_ = "untitled_effect.json";
            strncpy_s(saveAsFileName_, "untitled_effect.json", sizeof(saveAsFileName_) - 1);
            statusMessage_ = "新規エフェクトを作成しました。";
        }

        // 別名保存
        ImGui::SetNextItemWidth(180.0f);
        ImGui::InputText("##SaveAsName", saveAsFileName_, sizeof(saveAsFileName_));
        ImGui::SameLine();
        if (ImGui::Button("別名で保存 (Save As)"))
        {
            std::string saveName = saveAsFileName_;
            if (!saveName.ends_with(".json")) saveName += ".json";
            previewEffect_.SaveToJson(saveName);
            currentLoadedFile_ = saveName;
            RefreshFileList();
            statusMessage_ = "保存完了: " + saveName;
        }

        if (!statusMessage_.empty())
        {
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%s", statusMessage_.c_str());
        }

        // -------------------------------------------------------------
        // 2. プレビュー再生コントロール
        // -------------------------------------------------------------
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.3f, 0.85f, 1.0f, 1.0f), "■ 再生・カメラコントロール");
        ImGui::Separator();

        if (ImGui::Button("発射 (Play Burst)##Btn", ImVec2(140, 30)))
        {
            previewEffect_.Play();
        }
        ImGui::SameLine();
        ImGui::Checkbox("自動ループ再生", &autoLoop_);

        if (autoLoop_)
        {
            ImGui::SetNextItemWidth(120.0f);
            ImGui::SliderFloat("ループ間隔(s)", &loopInterval_, 0.2f, 5.0f, "%.1fs");
        }

        ImGui::SetNextItemWidth(120.0f);
        ImGui::SliderFloat("再生速度", &playbackSpeed_, 0.1f, 3.0f, "%.2fx");
        ImGui::SameLine();
        if (ImGui::SmallButton("1.0x")) playbackSpeed_ = 1.0f;
        ImGui::SameLine();
        if (ImGui::SmallButton("0.5x")) playbackSpeed_ = 0.5f;

        // 背景色
        float bgArr[4] = { backgroundColor_.x, backgroundColor_.y, backgroundColor_.z, backgroundColor_.w };
        if (ImGui::ColorEdit4("背景色", bgArr))
        {
            backgroundColor_ = Vector4(bgArr[0], bgArr[1], bgArr[2], bgArr[3]);
        }

        // 床グリッド設定
        ImGui::Spacing();
        ImGui::Checkbox("床グリッド線を表示", &showGrid_);
        if (showGrid_)
        {
            ImGui::SameLine();
            ImGui::Checkbox("原点XYZ軸", &showAxisGizmo_);

            ImGui::SetNextItemWidth(110.0f);
            ImGui::SliderFloat("グリッド範囲", &gridSize_, 3.0f, 30.0f, "%.0fm");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(90.0f);
            ImGui::SliderFloat("間隔", &gridSpacing_, 0.2f, 5.0f, "%.1fm");
        }

        if (ImGui::Button("カメラ位置リセット"))
        {
            cameraTarget_ = { 0.0f, 0.0f, 0.0f };
            cameraDistance_ = 6.0f;
            cameraYaw_ = 0.0f;
            cameraPitch_ = 0.35f;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("※操作: [右ドラッグ]回転 / [中ドラッグ]パン / [ホイール]ズーム");

        // -------------------------------------------------------------
        // 3. エフェクト詳細ノード編集
        // -------------------------------------------------------------
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.3f, 0.85f, 1.0f, 1.0f), "■ レイヤー・ノード編集 (Node Inspector)");
        ImGui::Separator();

        // 既存の DrawImGui 内容（ノード追加、ノード詳細調整）を表示
        // （DrawImGui の Begin/End なし版としてインライン描画、または直接ノードを操作）
        ImGui::Text("新しいレイヤーを追加:");
        static int newShapeType = 0;
        const char* shapes[] = { "Quad (平面・板)", "Ring (輪・リング)", "Cylinder (円筒)" };
        ImGui::SetNextItemWidth(160.0f);
        ImGui::Combo("形状##NewShape", &newShapeType, shapes, 3);
        ImGui::SameLine();
        if (ImGui::Button("レイヤー追加"))
        {
            previewEffect_.AddNode("New Layer", newShapeType);
        }

        ImGui::Separator();

        // 各ノードのパラメータ編集
        auto& nodes = previewEffect_.GetNodes();
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            auto& node = nodes[i];
            ImGui::PushID(static_cast<int>(i));

            std::string headerLabel = "[" + std::to_string(i + 1) + "] " + node->name;
            if (ImGui::CollapsingHeader(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::InputText("レイヤー名", node->name, sizeof(node->name));

                if (ImGui::Combo("形状タイプ", &node->shapeType, shapes, 3))
                {
                    previewEffect_.SetupRendererForNode(node.get());
                }

                auto param = node->emitter.GetParameter();
                bool changed = false;

                if (ImGui::TreeNode("発生パラメータ (Emitter Settings)"))
                {
                    int count = static_cast<int>(param.count);
                    if (ImGui::DragInt("発生数 (Count)", &count, 1, 0, 100)) { param.count = count; changed = true; }
                    if (ImGui::DragFloat("頻度 (Frequency)", &param.frequency, 0.01f, 0.0f, 2.0f)) { changed = true; }

                    float life[2] = { param.minLifeTime, param.maxLifeTime };
                    if (ImGui::DragFloat2("寿命 Min/Max", life, 0.05f, 0.0f, 10.0f))
                    {
                        param.minLifeTime = life[0]; param.maxLifeTime = life[1]; changed = true;
                    }
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("速度・スケール (Transform)"))
                {
                    if (ImGui::DragFloat3("初速 Min", &param.minVelocity.x, 0.05f)) changed = true;
                    if (ImGui::DragFloat3("初速 Max", &param.maxVelocity.x, 0.05f)) changed = true;
                    if (ImGui::DragFloat3("スケール Min", &param.minScale.x, 0.05f)) changed = true;
                    if (ImGui::DragFloat3("スケール Max", &param.maxScale.x, 0.05f)) changed = true;
                    if (ImGui::DragFloat3("回転 Min", &param.minRotation.x, 0.05f)) changed = true;
                    if (ImGui::DragFloat3("回転 Max", &param.maxRotation.x, 0.05f)) changed = true;
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("カラー・フェード (Color)"))
                {
                    if (ImGui::ColorEdit4("開始色 Min", &param.minColor.x)) changed = true;
                    if (ImGui::ColorEdit4("開始色 Max", &param.maxColor.x)) changed = true;

                    if (ImGui::Checkbox("寿命で色変化 (Color Over Lifetime)", &param.isColorOverLifetime)) changed = true;
                    if (param.isColorOverLifetime)
                    {
                        if (ImGui::ColorEdit4("終了色", &param.endColor.x)) changed = true;
                    }

                    if (ImGui::Checkbox("寿命で拡縮 (Scale Over Lifetime)", &param.isScaleOverLifetime)) changed = true;
                    if (param.isScaleOverLifetime)
                    {
                        if (ImGui::DragFloat3("終了時スケール", &param.endScale.x, 0.05f)) changed = true;
                    }
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("物理挙動 (Physics)"))
                {
                    if (ImGui::Checkbox("重力適用 (Use Gravity)", &param.useGravity)) changed = true;
                    if (param.useGravity)
                    {
                        if (ImGui::DragFloat("重力加速度", &param.gravity, 0.1f)) changed = true;
                    }
                    if (ImGui::DragFloat("空気抵抗 (Air Drag)", &param.drag, 0.01f, 0.0f, 1.0f)) changed = true;
                    ImGui::TreePop();
                }

                if (changed)
                {
                    node->emitter.SetParameter(param);
                }

                ImGui::Separator();
                ImGui::Text("マテリアル & ブレンドモード");

                char texName[64];
                strncpy_s(texName, node->emitter.GetTextureName().c_str(), sizeof(texName) - 1);
                if (ImGui::InputText("テクスチャ名", texName, sizeof(texName)))
                {
                    node->emitter.SetTextureName(texName);
                }

                int mode = static_cast<int>(node->emitter.GetBlendMode());
                const char* blendNames[] = { "None", "Alpha (半透明)", "Additive (加算)", "Multiply (乗算)", "PreMultiplied" };
                if (ImGui::Combo("ブレンドモード", &mode, blendNames, 5))
                {
                    node->emitter.SetBlendMode(static_cast<BlendMode>(mode));
                }

                ImGui::DragFloat("UVスクロール速度", &node->uvScrollSpeed, 0.05f, -5.0f, 5.0f);

                if (ImGui::Button("このレイヤーを削除"))
                {
                    nodes.erase(nodes.begin() + i);
                    ImGui::PopID();
                    break;
                }
            }
            ImGui::PopID();
        }
    }
    ImGui::End();
#endif
}

namespace
{
    // 3Dベクトル * Matrix4x4 (Row-Vector 形式の計算)
    inline Vector4 TransformPointToClip(const Vector3& p, const Matrix4x4& m)
    {
        Vector4 res;
        res.x = p.x * m.m[0][0] + p.y * m.m[1][0] + p.z * m.m[2][0] + m.m[3][0];
        res.y = p.x * m.m[0][1] + p.y * m.m[1][1] + p.z * m.m[2][1] + m.m[3][1];
        res.z = p.x * m.m[0][2] + p.y * m.m[1][2] + p.z * m.m[2][2] + m.m[3][2];
        res.w = p.x * m.m[0][3] + p.y * m.m[1][3] + p.z * m.m[2][3] + m.m[3][3];
        return res;
    }

    // Near 平面による線分クリップ (カメラ背後・視錐台外の補正)
    inline bool ClipLineToNear(Vector4& p0, Vector4& p1, float nearW = 0.05f)
    {
        bool p0In = (p0.w >= nearW);
        bool p1In = (p1.w >= nearW);
        if (!p0In && !p1In) return false;

        if (!p0In)
        {
            float t = (nearW - p0.w) / (p1.w - p0.w);
            p0.x = p0.x + t * (p1.x - p0.x);
            p0.y = p0.y + t * (p1.y - p0.y);
            p0.z = p0.z + t * (p1.z - p0.z);
            p0.w = nearW;
        }
        else if (!p1In)
        {
            float t = (nearW - p1.w) / (p0.w - p1.w);
            p1.x = p1.x + t * (p0.x - p1.x);
            p1.y = p1.y + t * (p0.y - p1.y);
            p1.z = p1.z + t * (p0.z - p1.z);
            p1.w = nearW;
        }
        return true;
    }

    // クリップ座標 -> スクリーン座標変換
    inline ImVec2 ClipToScreen(const Vector4& clip, const ImVec2& minPos, const ImVec2& size)
    {
        float invW = 1.0f / (clip.w > 0.0001f ? clip.w : 0.0001f);
        float ndcX = clip.x * invW;
        float ndcY = clip.y * invW;
        return ImVec2(
            minPos.x + (ndcX * 0.5f + 0.5f) * size.x,
            minPos.y + (-ndcY * 0.5f + 0.5f) * size.y
        );
    }
}

void EffectStudio::Draw3DLine(ImDrawList* drawList, const Vector3& start, const Vector3& end,
    const Matrix4x4& vpMat, const ImVec2& minPos, const ImVec2& size,
    uint32_t color, float thickness)
{
    Vector4 p0 = TransformPointToClip(start, vpMat);
    Vector4 p1 = TransformPointToClip(end, vpMat);

    if (!ClipLineToNear(p0, p1, 0.05f)) return;

    ImVec2 s0 = ClipToScreen(p0, minPos, size);
    ImVec2 s1 = ClipToScreen(p1, minPos, size);

    drawList->AddLine(s0, s1, color, thickness);
}

void EffectStudio::DrawGridOverlay(const ImVec2& imagePos, const ImVec2& imageSize)
{
    if (!showGrid_) return;

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->PushClipRect(imagePos, ImVec2(imagePos.x + imageSize.x, imagePos.y + imageSize.y), true);

    Matrix4x4 vpMat = camera_.GetViewMatrix() * camera_.GetProjectionMatrix();

    const float range = (std::max)(1.0f, gridSize_);
    const float step = (std::max)(0.1f, gridSpacing_);

    // 視認性の高いカラー定義
    ImU32 normalColor = IM_COL32(180, 185, 205, 55);   // 通常のグリッド線（半透明薄グレー）
    ImU32 majorColor  = IM_COL32(205, 215, 240, 105);  // 5本ごとの強調線
    ImU32 xAxisColor  = IM_COL32(245, 75, 75, 190);     // X軸（赤）
    ImU32 zAxisColor  = IM_COL32(75, 135, 255, 190);    // Z軸（青）
    ImU32 yAxisColor  = IM_COL32(75, 235, 95, 210);     // Y軸（緑）

    // 1. Z方向の平行線（X軸に沿って並ぶ線）
    for (float x = -range; x <= range + 0.001f; x += step)
    {
        bool isZero = std::abs(x) < 0.001f;
        int index = static_cast<int>(std::round(x / step));
        bool isMajor = !isZero && (index % 5 == 0);

        ImU32 col = isZero ? zAxisColor : (isMajor ? majorColor : normalColor);
        float th = isZero ? 2.0f : (isMajor ? 1.5f : 1.0f);

        Draw3DLine(drawList, { x, 0.0f, -range }, { x, 0.0f, range }, vpMat, imagePos, imageSize, col, th);
    }

    // 2. X方向の平行線（Z軸に沿って並ぶ線）
    for (float z = -range; z <= range + 0.001f; z += step)
    {
        bool isZero = std::abs(z) < 0.001f;
        int index = static_cast<int>(std::round(z / step));
        bool isMajor = !isZero && (index % 5 == 0);

        ImU32 col = isZero ? xAxisColor : (isMajor ? majorColor : normalColor);
        float th = isZero ? 2.0f : (isMajor ? 1.5f : 1.0f);

        Draw3DLine(drawList, { -range, 0.0f, z }, { range, 0.0f, z }, vpMat, imagePos, imageSize, col, th);
    }

    // 3. 原点XYZ軸ギズモ（中央から伸びる矢印風ライン）
    if (showAxisGizmo_)
    {
        const float axisLen = (std::min)(range * 0.35f, 3.0f);

        // X軸 (+X: 赤)
        Draw3DLine(drawList, { 0.0f, 0.0f, 0.0f }, { axisLen, 0.0f, 0.0f }, vpMat, imagePos, imageSize, xAxisColor, 2.5f);
        // Y軸 (+Y: 緑・上方向)
        Draw3DLine(drawList, { 0.0f, 0.0f, 0.0f }, { 0.0f, axisLen, 0.0f }, vpMat, imagePos, imageSize, yAxisColor, 2.5f);
        // Z軸 (+Z: 青)
        Draw3DLine(drawList, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, axisLen }, vpMat, imagePos, imageSize, zAxisColor, 2.5f);

        // 原点マーカー（白い丸点）
        Vector4 centerClip = TransformPointToClip({ 0.0f, 0.0f, 0.0f }, vpMat);
        if (centerClip.w > 0.05f)
        {
            ImVec2 centerScr = ClipToScreen(centerClip, imagePos, imageSize);
            drawList->AddCircleFilled(centerScr, 4.0f, IM_COL32(255, 255, 255, 220));
            drawList->AddCircle(centerScr, 6.0f, IM_COL32(255, 215, 0, 180), 12, 1.5f);
        }
    }

    drawList->PopClipRect();
}
