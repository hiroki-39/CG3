#include "EditorSystem.h"
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_internal.h"
#include "KHEngine/Graphics/Resource/Descriptor/SrvManager.h"
#include "KHEngine/Graphics/3d/Particle/ParticleManager.h"

EditorSystem* EditorSystem::GetInstance() {
    static EditorSystem instance;
    return &instance;
}

void EditorSystem::Initialize(DirectXCommon* dxCommon) {
    dxCommon_ = dxCommon;
}

void EditorSystem::Draw(uint32_t gameSceneSrvIndex) {
#ifdef USE_IMGUI
    // 1. メインのDockSpaceを作成
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    
    ImGui::Begin("DockSpace Demo", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    // DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");

        // 一度だけデフォルトのレイアウト（Unity風）を構築する（すでに保存されたレイアウトがある場合はスキップ）
        if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr) {
            // 既存のレイアウトをクリア
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

            // 画面を分割していく
            ImGuiID dock_main_id = dockspace_id;
            // 左側に「ヒエラルキー」的な領域 (20%)
            ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.20f, nullptr, &dock_main_id);
            // 右側に「インスペクター」的な領域 (25%)
            ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);
            // 下部に「コンソール・アセット」的な領域 (25%)
            ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.25f, nullptr, &dock_main_id);
            // 残った dock_main_id が中央 (Viewport)

            // 各ウィンドウを対応するドックに割り当てる
            ImGui::DockBuilderDockWindow("スプライト", dock_id_left);

            ImGui::DockBuilderDockWindow("モデル", dock_id_right);
            ImGui::DockBuilderDockWindow("カメラ", dock_id_right);
            ImGui::DockBuilderDockWindow("ライト", dock_id_right);
            ImGui::DockBuilderDockWindow("パーティクルエディタ", dock_id_right);

            // 下部にパフォーマンス（デバッグ）領域
            ImGui::DockBuilderDockWindow("パフォーマンス", dock_id_bottom);

            // 中央にはビューポート（ゲーム画面）
            ImGui::DockBuilderDockWindow("ゲーム画面", dock_main_id);

            // 構築完了
            ImGui::DockBuilderFinish(dockspace_id);
        }

        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    }

    DrawMenuBar();

    ImGui::End(); // DockSpace Demo

    // 各ウィンドウの描画 (DockSpaceのBegin/Endの外で行う必要があります)
    DrawViewport(gameSceneSrvIndex);
    
    if (showParticleEditor_) {
        DrawParticleEditor();
    }

    // パフォーマンス（FPSなど）ウィンドウ
    DrawPerformance();

    // --- シーン切り替え時のウィンドウ消滅防止 ---
    // シーン側で中身を描画しなくても「枠」だけは残るように、空のウィンドウを宣言しておきます
    ImGui::Begin("スプライト"); ImGui::End();
    ImGui::Begin("モデル"); ImGui::End();
    ImGui::Begin("カメラ"); ImGui::End();
    ImGui::Begin("ライト"); ImGui::End();
#endif
}

void EditorSystem::DrawMenuBar() {
#ifdef USE_IMGUI
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("ウィンドウ")) {
            ImGui::MenuItem("パーティクルエディタ", nullptr, &showParticleEditor_);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
#endif
}

void EditorSystem::DrawViewport(uint32_t srvIndex) {
#ifdef USE_IMGUI
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("ゲーム画面");
    {
        // ウィンドウのサイズに合わせて画像を表示
        ImVec2 contentSize = ImGui::GetContentRegionAvail();
        
        // SrvManagerからGPUハンドルを取得
        SrvManager* srvManager = SrvManager::GetInstance();
        D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = srvManager->GetSRVGPUDescriptorHandle(srvIndex);

        // 画像を表示 (UVを上下反転させる必要がある場合があります)
        ImGui::Image((ImTextureID)srvHandle.ptr, contentSize);
    }
    ImGui::End();
    ImGui::PopStyleVar();
#endif
}

void EditorSystem::DrawParticleEditor() {
#ifdef USE_IMGUI
    ImGui::Begin("パーティクルエディタ");
    
    ParticleManager* manager = ParticleManager::GetInstance();
    auto& emitters = manager->GetEmitters();

    static int selectedEmitter = -1;
    
    // エミッターリスト
    ImGui::BeginChild("EmitterList", ImVec2(150, 0), true);
    for (int i = 0; i < (int)emitters.size(); ++i) {
        std::string label = "エミッター " + std::to_string(i);
        if (ImGui::Selectable(label.c_str(), selectedEmitter == i)) {
            selectedEmitter = i;
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // パラメータ編集
    ImGui::BeginChild("Parameters");
    if (selectedEmitter >= 0 && selectedEmitter < (int)emitters.size()) {
        auto& emitter = emitters[selectedEmitter];
        auto param = emitter->GetParameter();

        bool changed = false;
        if (ImGui::CollapsingHeader("基本設定", ImGuiTreeNodeFlags_DefaultOpen)) {
            changed |= ImGui::DragInt("発生数", (int*)&param.count, 1, 1, 100);
            changed |= ImGui::DragFloat("発生頻度", &param.frequency, 0.01f, 0.0f, 10.0f);
        }

        if (ImGui::CollapsingHeader("寿命 (LifeTime)", ImGuiTreeNodeFlags_DefaultOpen)) {
            changed |= ImGui::DragFloat("最小寿命", &param.minLifeTime, 0.1f, 0.0f, 10.0f);
            changed |= ImGui::DragFloat("最大寿命", &param.maxLifeTime, 0.1f, 0.0f, 10.0f);
        }

        if (ImGui::CollapsingHeader("スケール", ImGuiTreeNodeFlags_DefaultOpen)) {
            changed |= ImGui::DragFloat3("最小スケール", &param.minScale.x, 0.1f);
            changed |= ImGui::DragFloat3("最大スケール", &param.maxScale.x, 0.1f);
        }

        if (ImGui::CollapsingHeader("速度 (Velocity)", ImGuiTreeNodeFlags_DefaultOpen)) {
            changed |= ImGui::DragFloat3("最小速度", &param.minVelocity.x, 0.1f);
            changed |= ImGui::DragFloat3("最大速度", &param.maxVelocity.x, 0.1f);
        }

        if (ImGui::CollapsingHeader("色 (Color)", ImGuiTreeNodeFlags_DefaultOpen)) {
            changed |= ImGui::ColorEdit4("開始色 (最小)", &param.minColor.x);
            changed |= ImGui::ColorEdit4("開始色 (最大)", &param.maxColor.x);
        }

        if (ImGui::CollapsingHeader("時間変化 (Over Lifetime)", ImGuiTreeNodeFlags_DefaultOpen)) {
            changed |= ImGui::Checkbox("時間で色を変化させる", &param.isColorOverLifetime);
            if (param.isColorOverLifetime) {
                changed |= ImGui::ColorEdit4("終了時の色", &param.endColor.x);
            }
            
            changed |= ImGui::Checkbox("時間でスケールを変化させる", &param.isScaleOverLifetime);
            if (param.isScaleOverLifetime) {
                changed |= ImGui::DragFloat3("終了時のスケール", &param.endScale.x, 0.1f);
            }
        }

        if (ImGui::CollapsingHeader("物理挙動 (Physics)", ImGuiTreeNodeFlags_DefaultOpen)) {
            changed |= ImGui::Checkbox("重力を有効化", &param.useGravity);
            if (param.useGravity) {
                changed |= ImGui::DragFloat("重力の強さ (Y方向)", &param.gravity, 0.1f);
            }
            changed |= ImGui::DragFloat("空気抵抗 (Drag)", &param.drag, 0.01f, 0.0f, 10.0f);
        }

        if (changed) {
            emitter->SetParameter(param);
        }
    } else {
        ImGui::Text("編集するエミッターを選択してください");
    }
    ImGui::EndChild();

    ImGui::End();
#endif
}

void EditorSystem::DrawPerformance() {
#ifdef USE_IMGUI
    ImGui::Begin("パフォーマンス");

    ImGuiIO& io = ImGui::GetIO();
    float fps = io.Framerate;
    float frameTime = 1000.0f / (fps > 0.0f ? fps : 1.0f);

    ImGui::Text("フレームレート (FPS): %.1f", fps);
    ImGui::Text("処理時間 (ms/frame): %.3f", frameTime);

    ImGui::Separator();

    // 簡易的なFPSの推移グラフ
    static float fps_values[90] = {};
    static int values_offset = 0;
    
    // 値を更新
    fps_values[values_offset] = fps;
    values_offset = (values_offset + 1) % IM_ARRAYSIZE(fps_values);

    ImGui::PlotLines("FPS 履歴", fps_values, IM_ARRAYSIZE(fps_values), values_offset, nullptr, 0.0f, 120.0f, ImVec2(0, 80));

    // もし将来的にGPUやCPUの正確な処理時間を計測するTimerクラスなどがあれば、
    // ここに ImGui::Text("GPU Draw: %.3f ms", gpuTime); のように追加できます。

    ImGui::End();
#endif
}

