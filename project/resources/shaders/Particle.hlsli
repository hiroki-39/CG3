#define float32_t4 float4
#define float32_t2 float2
#define float32_t3 float3

struct VertexShaderOutput
{
    float32_t4 position : SV_POSITION;
    float32_t2 texcoord : TEXCOORD0;
    float32_t4 color : COLOR0;
};
