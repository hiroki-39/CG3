#include "Object3d.hlsli"
#define float32_t4 float4
#define float32_t4x4 float4x4
#define int32_t int
#define float32_t float

struct Material
{
    float32_t4 color;
    bool enableLighting;
    float32_t4x4 uvTransform;
    int32_t selectLightings;
    float32_t shininess;
    float32_t3 specularColor;
};

struct DirectionlLight
{
    float32_t4 color;
    float32_t3 direction;
    float intensity;
};

struct PointLight
{
    float32_t4 color;
    float32_t3 position;
    float intensity;
    float radius;
    float decry;
};

struct SpotLight
{
    float32_t4 color;
    float32_t3 position;
    float32_t intensity;
    float32_t3 direction;
    float32_t distance;
    float32_t decay;
    float32_t cosAngle;
};

struct Camera
{
   float32_t3 worldPosition;
};

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionlLight> gDirectionlLight : register(b1);
ConstantBuffer<Camera> gCamera : register(b2);
ConstantBuffer<PointLight> gPointLight : register(b3);
ConstantBuffer<SpotLight> gSpotLight : register(b4);

TextureCube<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float32_t4 textureColor = gTexture.sample(input.texcoord);
    output.color = textureColor * gMaterial.color;
 
    return output;
}