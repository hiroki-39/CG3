#include"Particle.hlsli"
#define float32_t4 float4
#define float32_t4x4 float4x4
#define int32_t int


struct Material
{
    float32_t4 color;
    bool enableLighting;
    float32_t4x4 uvTransform;
    int32_t selectLightings;
};

struct DirectionlLight
{
    float32_t4 color;
    float32_t3 direction;
    float intensity;
};

ConstantBuffer<Material> gMaterial : register(b0);

ConstantBuffer<DirectionlLight> gDirectionlLight : register(b1);

Texture2D<float32_t4> gTexture : register(t1);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float32_t4 transformedUV = mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float32_t4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    output.color = gMaterial.color * textureColor;

    if(output.color.a == 0.0f)
    {
        discard;
    }    

    return output;
}