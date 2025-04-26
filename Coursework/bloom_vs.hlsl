/** Bloom: Vertex Shader Code **/

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
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
};

/****************************************************************************************************************************/

// Struct to define the output to the vertex shader
struct OutputType
{
    float4 position : SV_POSITION; // Transformed vertex position to be passed to the pixel shader
    float2 tex : TEXCOORD0; // Unmodified texture coordinates passed to the pixel shader
};

/****************************************************************************************************************************/

// Main vertex shader function
OutputType main(InputType input)
{
    OutputType output;
    
    output.position = mul(input.position, worldMatrix); // Transform position to world space
    output.position = mul(output.position, viewMatrix); // to camera view space
    output.position = mul(output.position, projectionMatrix); // to screen space
    
    output.tex = input.tex;

    return output;
}
