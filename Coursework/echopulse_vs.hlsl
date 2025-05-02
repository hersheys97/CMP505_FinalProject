/** EchoPulse: Vertex Shader Code **/

/****************************************************************************************************************************/

// Constant buffer for storing transformation matrices
cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix; // world space
    matrix viewMatrix; // camera view space
    matrix projectionMatrix; // view space coordinates to screen space
};

/****************************************************************************************************************************/

// Struct to define the input to the vertex shader
struct InputType
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
};

/****************************************************************************************************************************/

// Struct to define the output to the vertex shader
struct OutputType
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

/****************************************************************************************************************************/

// Main vertex shader function
OutputType main(InputType input)
{
    OutputType output;
    
    output.position = mul(float4(input.position, 1.0f), worldMatrix); // Transform position to world space
    output.position = mul(output.position, viewMatrix); // to camera view space
    output.position = mul(output.position, projectionMatrix); // to screen space
    
    output.texCoord = input.texCoord;

    return output;
}
