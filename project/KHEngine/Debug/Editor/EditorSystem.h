#pragma once
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include "externals/imgui/imgui.h"
#include <cstdint>

/**
 * @brief エディタ機能を管理するシステムクラス
 * Unityのようなレイアウト構成（ドッキング空間、ビューポート）を提供します。
 */
class EditorSystem {
public:
    static EditorSystem* GetInstance();
    
    // 初期化
    void Initialize(DirectXCommon* dxCommon);
    
    // ImGui描画処理。メインループのImGui描画セクションで呼び出してください。
    void Draw(uint32_t gameSceneSrvIndex);

    // ゲーム画面ビューポートのスクリーン座標・サイズ（UIクリッピング等用）
    ImVec2 GetViewportPos() const { return viewportPos_; }
    ImVec2 GetViewportSize() const { return viewportSize_; }

private:
    EditorSystem() = default;
    ~EditorSystem() = default;
    EditorSystem(const EditorSystem&) = delete;
    EditorSystem& operator=(const EditorSystem&) = delete;

    // 各ウィンドウの描画
    void DrawMenuBar();
    void DrawViewport(uint32_t srvIndex);
    void DrawParticleEditor();
    void DrawPerformance();

    DirectXCommon* dxCommon_ = nullptr;
    bool showParticleEditor_ = false;
    bool showUITextEditor_ = true;
    bool resetLayout_ = false;

    ImVec2 viewportPos_ = { 0.0f, 0.0f };
    ImVec2 viewportSize_ = { 1280.0f, 720.0f };
};
