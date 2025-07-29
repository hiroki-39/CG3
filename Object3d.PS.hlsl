#include"Object3d.hlsli"

struct Material
{
    float32_t4 color;
    bool enableLighting;
    float32_t4x4 uvTransform;
    int32_t serectLighting;
};

struct DirectionlLight
{
    float32_t4 color;
    float32_t3 direction;
    float intensity;
};

ConstantBuffer<Material> gMaterial : register(b0);

ConstantBuffer<DirectionlLight> gDirectionlLight : register(b1);

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
    
    float cos;
    float NdotL;
    
       if(gMaterial.enableLighting != 0)
    {
          cos = saturate(dot(normalize(input.normal), -gDirectionlLight.direction));
            output.color = gMaterial.color * textureColor * gDirectionlLight.color * cos * gDirectionlLight.intensity;
    }
    else
    {
        output.color = gMaterial.color * textureColor; 
    }
    
    
   
    return output;
}

