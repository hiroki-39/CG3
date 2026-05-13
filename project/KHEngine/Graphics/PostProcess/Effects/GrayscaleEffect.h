#pragma once
#include "KHEngine/Graphics/PostProcess/IPostProcess.h"
#include "externals/imgui/imgui.h"

class GrayscaleEffect : public IPostProcess {
public:
    GrayscaleEffect(PostProcessData* data) : data_(data) {
        name_ = "Grayscale";
    }

    void Initialize() override {
        // 初期化処理があればここに
    }

    void DrawImGui() override {
        // グレースケールは特に調整パラメータがないのでテキストのみ
        ImGui::Text("Grayscale is Active.");
    }

private:
    PostProcessData* data_;
};
