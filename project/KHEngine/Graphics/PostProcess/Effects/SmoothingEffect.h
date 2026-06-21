#pragma once
#include "KHEngine/Graphics/PostProcess/IPostProcess.h"
#include "externals/imgui/imgui.h"

class SmoothingEffect : public IPostProcess {
public:
    SmoothingEffect(PostProcessData* data) : data_(data) {
        name_ = "Smoothing";
    }

    void Initialize() override {
        data_->smoothingKernelSize = 2.0f;
    }

    void DrawImGui() override {
#ifdef USE_IMGUI
        ImGui::SliderFloat("Kernel Size", &data_->smoothingKernelSize, 1.0f, 10.0f);
#endif
    }

private:
    PostProcessData* data_;
};
