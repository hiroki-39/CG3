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
    float32_t3 specularColor; // 追加: 鏡面反射色（資料の float32_t3(1,1,1) をマテリアルで設定可能に）
};

struct DirectionlLight
{
    float32_t4 color;
    float32_t3 direction;
    float intensity;
};

struct Camera
{
   float32_t3 worldPosition;
};

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionlLight> gDirectionlLight : register(b1);
ConstantBuffer<Camera> gCamera : register(b2);

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 transformedUV = mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float32_t4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    
    
    float32_t3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
    float32_t3 reflectLight = reflect(gDirectionlLight.direction, normalize(input.normal));
    float RdotE = dot(reflectLight, toEye);
    
    float32_t3 halfVector = normalize(-gDirectionlLight.direction + toEye);
    float NdotH = dot(normalize(input.normal), halfVector);
    float specularPow = pow(saturate(NdotH), gMaterial.shininess);
    
    float cos;
    float NdotL;
    
    switch (gMaterial.selectLightings)
    {
        case 0:
            output.color = gMaterial.color * textureColor;
            
            break;
        case 1:
        
            cos = saturate(dot(normalize(input.normal), -gDirectionlLight.direction));
            output.color.rgb = gMaterial.color.rgb * textureColor.rgb * gDirectionlLight.color.rgb * cos * gDirectionlLight.intensity;
            output.color.a = gMaterial.color.a * textureColor.a;
            
            break;
        case 2:
        
            NdotL = dot(normalize(input.normal), -gDirectionlLight.direction);
            cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
            output.color.rgb = gMaterial.color.rgb * textureColor.rgb * gDirectionlLight.color.rgb * cos * gDirectionlLight.intensity;
            output.color.a = gMaterial.color.a * textureColor.a;
        
            break;
        case 3:
  
            NdotL = dot(normalize(input.normal), -gDirectionlLight.direction);
            cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
                float32_t3 diffuse = gMaterial.color.rgb * textureColor.rgb * gDirectionlLight.color.rgb * cos * gDirectionlLight.intensity;
                float32_t3 specular = gDirectionlLight.color.rgb * gDirectionlLight.intensity * specularPow * float32_t3(1.0f, 1.0f, 1.0f);
            output.color.rgb = diffuse + specular;
            output.color.a = gMaterial.color.a * textureColor.a;
            break;
    }

    return output;
}