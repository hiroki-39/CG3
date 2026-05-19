#pragma once
#include "KHEngine/Graphics/PostProcess/IPostProcess.h"
#include "externals/imgui/imgui.h"

class SepiaEffect : public IPostProcess {
public:
    SepiaEffect(PostProcessData* data) : data_(data) {
        name_ = "Sepia";
    }

    void Initialize() override {
        data_->sepiaStrength = 0.8f; // 初期値
    }

    void DrawImGui() override {
        ImGui::SliderFloat("Sepia Strength", &data_->sepiaStrength, 0.0f, 1.0f);
    }

private:
    PostProcessData* data_;
};
