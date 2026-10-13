#include "Skybox.hlsli"

#define float32_t4 float4
#define float32_t3 float3
#define float32_t2 float2
#define float32_t4x4 float4x4

struct TransformationMatrix
{
    float32_t4x4 WVP;
    float32_t4x4 World;
    float32_t4x4 WorldInverseTranspose;
    int isVirtualSpace;
    float32_t3 padding;
};

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);
TextureCube<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

float hash31(float3 p)
{
    p = frac(p * float3(443.897, 441.423, 437.195));
    p += dot(p, p.yzx + 19.19);
    return frac((p.x + p.y) * p.z);
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float32_t4 textureColor = gTexture.Sample(gSampler, input.texcoord);

    if (gTransformationMatrix.isVirtualSpace != 0)
    {
        float3 dir = normalize(input.texcoord);

        // 1. 深宇宙ダークティール〜ネイビーの背景ベース
        float3 spaceBase = lerp(float3(0.01f, 0.08f, 0.10f), float3(0.005f, 0.015f, 0.035f), saturate(dir.y * 1.4f + 0.2f));

        // 2. 地平線（Horizon）の強烈なエメラルド・シアングローライン（参考画像の象徴的な水平発光帯）
        float horizonDist = abs(dir.y + 0.015f);
        float horizonCore = exp(-horizonDist * 38.0f);
        float horizonHalo = exp(-horizonDist * 7.5f);
        float3 horizonColor = float3(0.45f, 1.0f, 0.92f) * (horizonCore * 1.35f) + float3(0.0f, 0.72f, 0.62f) * (horizonHalo * 0.75f);

        // 3. 上空の幻想的なオーロラカーテン（エメラルドグリーン ＆ マゼンタレッドの揺らめく帯）
        float3 auroraColor = float3(0.0f, 0.0f, 0.0f);
        if (dir.y > -0.05f)
        {
            float heightMask = smoothstep(-0.02f, 0.15f, dir.y) * smoothstep(0.92f, 0.22f, dir.y);
            float u = atan2(dir.x, dir.z) * 3.5f;
            float wave1 = sin(u * 2.2f + sin(u * 0.9f) * 1.8f + dir.y * 4.5f);
            float wave2 = cos(u * 3.4f - sin(u * 1.5f) * 1.4f - dir.y * 3.2f);
            float curtain1 = pow(saturate(wave1 * 0.5f + 0.5f), 2.8f);
            float curtain2 = pow(saturate(wave2 * 0.5f + 0.5f), 3.2f);

            // 縦方向の筋（オーロラのストリーク感）
            float streaks = 0.65f + 0.35f * sin(u * 18.0f + wave1 * 3.0f);

            float3 emeraldAurora = float3(0.08f, 0.92f, 0.58f) * curtain1 * streaks * 0.68f;
            float3 magentaAurora = float3(0.78f, 0.14f, 0.36f) * curtain2 * streaks * 0.52f;
            auroraColor = (emeraldAurora + magentaAurora) * heightMask;
        }

        // 4. 微細な星空（Starfield）
        float3 starGrid = floor(dir * 220.0f);
        float starVal = hash31(starGrid);
        float starBright = smoothstep(0.994f, 1.0f, starVal) * smoothstep(-0.02f, 0.18f, dir.y);
        float3 stars = float3(0.75f, 0.98f, 1.0f) * starBright * 0.85f;

        // 5. 地面側（dir.y < 0）もダークエメラルドに統一
        float3 finalSky = spaceBase + auroraColor + horizonColor + stars;
        output.color = float32_t4(finalSky, 1.0f);
        return output;
    }

    output.color = textureColor;
    return output;
}