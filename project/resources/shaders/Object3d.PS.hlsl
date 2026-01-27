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
    
    float32_t3 normal = normalize(input.normal);
    float32_t3 toEye = normalize(gCamera.worldPosition - input.worldPosition);

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
             // DirectionalLight
                NdotL = dot(normal, -gDirectionlLight.direction);
                cos = pow(NdotL * 0.5f + 0.5f, 2.0f);

             float32_t3 diffuseDirectionalLight = gMaterial.color.rgb * textureColor.rgb * gDirectionlLight.color.rgb * cos * gDirectionlLight.intensity;
             float32_t3 specularDirectionalLight = gDirectionlLight.color.rgb * gDirectionlLight.intensity * specularPow * float32_t3(1.0f, 1.0f, 1.0f);
                
             // ===== PointLight =====
             float32_t3 pointLightDirection = normalize(input.worldPosition - gPointLight.position);

            float32_t distance = length(gPointLight.position - input.worldPosition);
            float32_t factor = pow(saturate(-distance / gPointLight.radius + 1.0), gPointLight.decry);
            
                // Diffuse
                float NdotL_Point = dot(normal, -pointLightDirection);
                float cosPoint = pow(NdotL_Point * 0.5f + 0.5f, 2.0f);

            float32_t3 diffusePointLight = gMaterial.color.rgb * textureColor.rgb * gPointLight.color.rgb * cosPoint * gPointLight.intensity * factor;

            // Blinn-Phong Specular
            float32_t3 halfVectorPoint = normalize(-pointLightDirection + toEye);

                float NdotH_Point = dot(normal, halfVectorPoint);
                float specularPowPoint = pow(saturate(NdotH_Point), gMaterial.shininess);

            float32_t3 specularPointLight = gPointLight.color.rgb * gPointLight.intensity * specularPowPoint * float32_t3(1.0f, 1.0f, 1.0f) * factor;

                // ===== 全部足す =====
                output.color.rgb = diffuseDirectionalLight + specularDirectionalLight + diffusePointLight + specularPointLight;

                output.color.a = gMaterial.color.a * textureColor.a;
            }
            break;
        case 5:
            {
             // ===== SpotLight =====
            
             // 表面 → 光源方向
            float32_t3 spotLightDirection = normalize(input.worldPosition - gSpotLight.position);

            // 距離減衰
            float32_t distance = length(gSpotLight.position - input.worldPosition);

            float32_t attenuation = pow(saturate(-distance / gSpotLight.distance + 1.0f), gSpotLight.decay);

                // 角度（Falloff）
                float cosAngle = dot(spotLightDirection, gSpotLight.direction);

                float falloffFactor = saturate((cosAngle - gSpotLight.cosAngle) / (1.0f - gSpotLight.cosAngle));

             // ===== Diffuse =====
                float NdotL = dot(normal, -spotLightDirection);

                float cosDiffuse = pow(NdotL * 0.5f + 0.5f, 2.0f);

           float32_t3 diffuseSpotLight = gMaterial.color.rgb * textureColor.rgb * gSpotLight.color.rgb * cosDiffuse * gSpotLight.intensity * attenuation * falloffFactor;

           // ===== Specular (Blinn-Phong) =====
           float32_t3 halfVector = normalize(-spotLightDirection + toEye);

                float NdotH = dot(normal, halfVector);

                float specularPow = pow(saturate(NdotH), gMaterial.shininess);

           float32_t3 specularSpotLight = gSpotLight.color.rgb * gSpotLight.intensity * specularPow * float32_t3(1.0f, 1.0f, 1.0f) * attenuation * falloffFactor;

                // ===== 出力 =====
                output.color.rgb = diffuseSpotLight + specularSpotLight;

                output.color.a = gMaterial.color.a * textureColor.a;
            }
            break;
    }

    return output;
}