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
ConstantBuffer<PointLight> gPointLight : register(b3);

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
    
    // 基本ベクトル類
    float32_t3 normal = normalize(input.normal);
    float32_t3 toEye = normalize(gCamera.worldPosition - input.worldPosition);

    // Directional 用の既存計算（そのまま残す）
    float32_t3 reflectLight = reflect(gDirectionlLight.direction, normal);
    float RdotE = dot(reflectLight, toEye);
    
    float32_t3 halfVector = normalize(-gDirectionlLight.direction + toEye);
    float NdotH = dot(normal, halfVector);
    float specularPow = pow(saturate(NdotH), gMaterial.shininess);
    
    float cos;
    float NdotL;
    
    switch (gMaterial.selectLightings)
    {
        case 0:
            output.color = gMaterial.color * textureColor;
            break;
        case 1:
            cos = saturate(dot(normal, -gDirectionlLight.direction));
            output.color.rgb = gMaterial.color.rgb * textureColor.rgb * gDirectionlLight.color.rgb * cos * gDirectionlLight.intensity;
            output.color.a = gMaterial.color.a * textureColor.a;
            break;
        case 2:
            NdotL = dot(normal, -gDirectionlLight.direction);
            cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
            output.color.rgb = gMaterial.color.rgb * textureColor.rgb * gDirectionlLight.color.rgb * cos * gDirectionlLight.intensity;
            output.color.a = gMaterial.color.a * textureColor.a;
            break;
        case 3:
            NdotL = dot(normal, -gDirectionlLight.direction);
            cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
            {
                float32_t3 diffuse = gMaterial.color.rgb * textureColor.rgb * gDirectionlLight.color.rgb * cos * gDirectionlLight.intensity;
                float32_t3 specular = gDirectionlLight.color.rgb * gDirectionlLight.intensity * specularPow * float32_t3(1.0f, 1.0f, 1.0f);
                output.color.rgb = diffuse + specular;
                output.color.a = gMaterial.color.a * textureColor.a;
            }
            break;
        case 4:
        {
            // PointLight 実装（位置 = gPointLight.direction）
            float32_t3 lightPos = gPointLight.direction;
            float32_t3 Lvec = lightPos - input.worldPosition;
            float32_t distance = length(Lvec);
            float32_t3 L = normalize(Lvec);

            // 距離減衰（簡易）、必要なら定数/線形/二次係数を導入してください
            float32_t attenuation = 1.0f / (1.0f + distance * distance);

            // Lambert diffuse
            float32_t nL = saturate(dot(normal, L));
            float32_t3 diffuse = gMaterial.color.rgb * textureColor.rgb * gPointLight.color.rgb * nL * gPointLight.intensity * attenuation;

            // Blinn-Phong specular
            float32_t3 halfVecPL = normalize(L + toEye);
            float32_t nH = saturate(dot(normal, halfVecPL));
            float32_t specPL = pow(nH, gMaterial.shininess);
            float32_t3 specular = gPointLight.color.rgb * gPointLight.intensity * specPL * gMaterial.specularColor;

                output.color.rgb = diffuse + specular;

            // alpha: 通常は material.a * texture.a を使うが、テクスチャが完全に透明な場合に
            // シーン全体が消える問題を防ぐため小さなフォールバックを行う
            float32_t outAlpha = gMaterial.color.a * textureColor.a;
                if (outAlpha < 0.001f)
                {
                    outAlpha = 1.0f; // 必要に応じてこの動作は変更してください
                }
                output.color.a = gMaterial.color.a * textureColor.a;
            }
            break;
    }

    return output;
}