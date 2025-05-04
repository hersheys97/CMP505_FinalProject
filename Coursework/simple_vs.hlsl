cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix; // world space
    matrix viewMatrix; // camera view space
    matrix projectionMatrix; // view space coordinates to screen space
};

struct InputType
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
};

struct OutputType
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

OutputType main(InputType input)
{
    OutputType output;
    
    output.position = mul(float4(input.position, 1.0f), worldMatrix); // Transform position to world space
    output.position = mul(output.position, viewMatrix); // to camera view space
    output.position = mul(output.position, projectionMatrix); // to screen space
    
    output.texCoord = input.texCoord;

    return output;
}