#pragma once
#include "KHEngine/Graphics/PostProcess/IPostProcess.h"
#include "externals/imgui/imgui.h"

class RandomEffect : public IPostProcess {
public:
    RandomEffect(PostProcessData* data) : data_(data) {
        name_ = "Random(Glitch)";
    }

    void Initialize() override {
        data_->glitchStrength = 0.05f;
        data_->noiseStrength = 0.1f;
    }

    void DrawImGui() override {
        ImGui::SliderFloat("Glitch Strength", &data_->glitchStrength, 0.0f, 0.2f);
        ImGui::SliderFloat("Noise Strength (Sandstorm)", &data_->noiseStrength, 0.0f, 1.0f);
    }

private:
    PostProcessData* data_;
};
