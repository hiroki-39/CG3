struct TransformationMatrix
{
    float32_t4x4 WVP;
};

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

struct VertexShaderQutput
{
    float32_t4 position : SV_POSITION;
};



struct VertexShaderInput
{
    float32_t4 position : POSITION0;
};

VertexShaderQutput main(VertexShaderInput input)
{
    VertexShaderQutput output;
    output.position = mul(input.position,gTransformationMatrix.WVP);
    return output;
}