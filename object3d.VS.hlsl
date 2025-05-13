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
    output.position = input.position;
    return output;
}