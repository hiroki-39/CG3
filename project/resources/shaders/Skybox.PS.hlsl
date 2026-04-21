#include "Skybox.hlsli"

#define float32_t4 float4

cbuffer Material : register(b0)
{
    float32_t4 color;
};

TextureCube<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float32_t4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    output.color = textureColor * color;
    return output;
}