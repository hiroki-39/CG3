#include "CopyImage.hlsli"

#define float32_t4 float4

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// C++側の PostProcessData と一致する定数バッファ
cbuffer PostProcessBuffer : register(b0) {
    int enableGrayscale;
    int enableSepia;
    float sepiaStrength;
    float padding;
}

struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    float32_t4 color = gTexture.Sample(gSampler, input.texcoord);
    
    // RGBから輝度（グレー）を計算
    float gray = dot(color.rgb, float3(0.299, 0.587, 0.114));

    if (enableGrayscale != 0) {
        // グレースケール
        color.rgb = float3(gray, gray, gray);
    }
    else if (enableSepia != 0) {
        // セピア
        float3 sepiaColor = float3(gray * 1.0, gray * 0.8, gray * 0.5);
        color.rgb = lerp(color.rgb, sepiaColor, sepiaStrength);
    }
    
    output.color = color;
    return output;
}
