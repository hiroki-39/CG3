#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
Texture2D<float4> gMaskTexture : register(t1); // Dissolve等のマスク用
SamplerState gSampler : register(s0);

// C++側の PostProcessData と完全に一致させる
cbuffer PostProcessBuffer : register(b0) {
    int enableGrayscale;
    int enableSepia;
    float sepiaStrength;
    int enableVignette;

    float vignetteIntensity;
    float vignettePower;
    int enableSmoothing;
    float smoothingKernelSize;

    int enableGaussian;
    float gaussianSigma;
    int enableOutline;
    float outlineThreshold;

    int enableRadialBlur;
    float radialBlurCenterX;
    float radialBlurCenterY;
    float radialBlurIntensity;

    int enableDissolve;
    float dissolveThreshold;
    float dissolveEdgeWidth;
    float padding1;

    float3 dissolveEdgeColor;
    int enableRandom;

    float randomTime;
    float glitchStrength;
    float noiseStrength;
    float padding2;
}

// 疑似乱数ジェネレーター
float random(float2 st) {
    return frac(sin(dot(st.xy, float2(12.9898, 78.233))) * 43758.5453123);
}

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    float2 uv = input.texcoord;

    // 1. Random (Glitch) - UVをずらす
    if (enableRandom != 0) {
        // 横方向へのスライスズレ
        float noiseY = random(float2(floor(uv.y * 50.0), randomTime));
        if (noiseY > 1.0 - glitchStrength) {
            uv.x += (random(float2(uv.y, randomTime)) - 0.5) * 0.1 * glitchStrength * 10.0;
        }
    }

    float4 color = gTexture.Sample(gSampler, uv);

    // Glitchの色収差(RGBずらし)
    if (enableRandom != 0 && glitchStrength > 0.0) {
        float offset = glitchStrength * 0.05 * random(float2(randomTime, randomTime));
        float r = gTexture.Sample(gSampler, uv + float2(offset, 0)).r;
        float b = gTexture.Sample(gSampler, uv - float2(offset, 0)).b;
        color.r = r;
        color.b = b;
    }

    // 2. Blur (Smoothing / Gaussian / RadialBlur)
    if (enableSmoothing != 0) {
        float3 sum = float3(0,0,0);
        float kernel = max(1.0, smoothingKernelSize);
        int k = (int)kernel;
        float weight = 1.0 / pow((k * 2.0 + 1.0), 2.0);
        float2 texelSize = float2(1.0/1280.0, 1.0/720.0);
        
        for (int y = -k; y <= k; ++y) {
            for (int x = -k; x <= k; ++x) {
                sum += gTexture.Sample(gSampler, uv + float2(x, y) * texelSize).rgb * weight;
            }
        }
        color.rgb = sum;
    }
    else if (enableGaussian != 0) {
        float3 sum = float3(0,0,0);
        float totalWeight = 0.0;
        int k = 3; // 固定カーネルサイズ
        float2 texelSize = float2(1.0/1280.0, 1.0/720.0) * gaussianSigma;

        for (int y = -k; y <= k; ++y) {
            for (int x = -k; x <= k; ++x) {
                float weight = exp(-(x*x + y*y) / (2.0 * gaussianSigma * gaussianSigma));
                sum += gTexture.Sample(gSampler, uv + float2(x, y) * texelSize).rgb * weight;
                totalWeight += weight;
            }
        }
        color.rgb = sum / totalWeight;
    }
    else if (enableRadialBlur != 0) {
        float2 center = float2(radialBlurCenterX, radialBlurCenterY);
        float2 dir = center - uv;
        float3 sum = float3(0,0,0);
        int samples = 10;
        for(int i = 0; i < samples; i++) {
            float t = (float)i / (float)samples;
            sum += gTexture.Sample(gSampler, uv + dir * t * radialBlurIntensity).rgb;
        }
        color.rgb = sum / samples;
    }

    // 3. Outline (簡易ソーベル)
    if (enableOutline != 0) {
        float2 texelSize = float2(1.0/1280.0, 1.0/720.0);
        
        float c00 = dot(gTexture.Sample(gSampler, uv + float2(-1, -1)*texelSize).rgb, float3(0.3,0.6,0.1));
        float c10 = dot(gTexture.Sample(gSampler, uv + float2( 0, -1)*texelSize).rgb, float3(0.3,0.6,0.1));
        float c20 = dot(gTexture.Sample(gSampler, uv + float2( 1, -1)*texelSize).rgb, float3(0.3,0.6,0.1));
        
        float c01 = dot(gTexture.Sample(gSampler, uv + float2(-1,  0)*texelSize).rgb, float3(0.3,0.6,0.1));
        float c21 = dot(gTexture.Sample(gSampler, uv + float2( 1,  0)*texelSize).rgb, float3(0.3,0.6,0.1));
        
        float c02 = dot(gTexture.Sample(gSampler, uv + float2(-1,  1)*texelSize).rgb, float3(0.3,0.6,0.1));
        float c12 = dot(gTexture.Sample(gSampler, uv + float2( 0,  1)*texelSize).rgb, float3(0.3,0.6,0.1));
        float c22 = dot(gTexture.Sample(gSampler, uv + float2( 1,  1)*texelSize).rgb, float3(0.3,0.6,0.1));

        float sx = -c00 + c20 - 2.0*c01 + 2.0*c21 - c02 + c22;
        float sy = -c00 - 2.0*c10 - c20 + c02 + 2.0*c12 + c22;
        float edge = sqrt(sx*sx + sy*sy);

        if (edge > outlineThreshold) {
            color.rgb = float3(0,0,0); // アウトラインは黒
        }
    }

    // 4. Color (Grayscale / Sepia)
    float gray = dot(color.rgb, float3(0.299, 0.587, 0.114));
    if (enableGrayscale != 0) {
        color.rgb = float3(gray, gray, gray);
    }
    else if (enableSepia != 0) {
        float3 sepiaColor = float3(gray * 1.0, gray * 0.8, gray * 0.5);
        color.rgb = lerp(color.rgb, sepiaColor, sepiaStrength);
    }

    // 5. Vignette
    if (enableVignette != 0) {
        float dist = distance(uv, float2(0.5, 0.5));
        float v = smoothstep(0.8, 0.2, dist * vignettePower);
        color.rgb *= lerp(1.0, v, vignetteIntensity);
    }

    // 6. Dissolve
    if (enableDissolve != 0) {
        float maskVal = gMaskTexture.Sample(gSampler, input.texcoord).r;
        
        if (maskVal < dissolveThreshold) {
            discard; // 閾値以下は透明に抜く
        } 
        else if (maskVal < dissolveThreshold + dissolveEdgeWidth) {
            color.rgb = dissolveEdgeColor; // エッジ部分の色
        }
    }

    // 7. Random (砂嵐 Noise)
    if (enableRandom != 0 && noiseStrength > 0.0) {
        float noise = random(uv * randomTime);
        color.rgb += (noise - 0.5) * noiseStrength;
    }

    output.color = color;
    return output;
}
