/** Vertex Shader Code **/

/****************************************************************************************************************************/

// Constant buffer for storing transformation matrices
cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix orthoView;
    matrix orthoMatrix;
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
    
    output.position = float4(input.position, 1.f);
    output.texCoord = input.texCoord;

    return output;
}
