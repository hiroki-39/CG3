#pragma once
#include "KHEngine/Graphics/PostProcess/IPostProcess.h"
#include "externals/imgui/imgui.h"

class RadialBlurEffect : public IPostProcess {
public:
    RadialBlurEffect(PostProcessData* data) : data_(data) {
        name_ = "RadialBlur";
    }

    void Initialize() override {
        data_->radialBlurCenterX = 0.5f;
        data_->radialBlurCenterY = 0.5f;
        data_->radialBlurIntensity = 0.02f;
    }

    void DrawImGui() override {
        ImGui::SliderFloat("Center X", &data_->radialBlurCenterX, 0.0f, 1.0f);
        ImGui::SliderFloat("Center Y", &data_->radialBlurCenterY, 0.0f, 1.0f);
        ImGui::SliderFloat("Intensity", &data_->radialBlurIntensity, 0.0f, 0.1f);
    }

private:
    PostProcessData* data_;
};
