#pragma once
#include "KHEngine/Graphics/PostProcess/IPostProcess.h"
#include "externals/imgui/imgui.h"

class OutlineEffect : public IPostProcess {
public:
    OutlineEffect(PostProcessData* data) : data_(data) {
        name_ = "Outline";
    }

    void Initialize() override {
        data_->outlineThreshold = 0.5f;
    }

    void DrawImGui() override {
#ifdef USE_IMGUI
        ImGui::SliderFloat("Threshold", &data_->outlineThreshold, 0.0f, 2.0f);
#endif
    }

private:
    PostProcessData* data_;
};
