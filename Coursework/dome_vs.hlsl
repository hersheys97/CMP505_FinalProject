/** Dome: Vertex Shader Code **/

/****************************************************************************************************************************/

// Constant buffer for storing transformation matrices
cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix; // world space
    matrix viewMatrix; // camera view space
    matrix projectionMatrix; // view space coordinates to screen space
    matrix lightViewMatrix; // light's view space (for shadow mapping)
    matrix lightProjectionMatrix; // light view space positions to screen space (for shadow mapping)
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
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float4 lightViewPos : TEXCOORD1;
};

/****************************************************************************************************************************/

// Main vertex shader function
OutputType main(InputType input)
{
    OutputType output;
    
    output.position = mul(input.position, worldMatrix); // Transform position to world space
    output.position = mul(output.position, viewMatrix); // to camera view space
    output.position = mul(output.position, projectionMatrix); // to screen space
    
    // Calculate the position of the vertice as viewed by the light source.
    output.lightViewPos = mul(input.position, worldMatrix);
    output.lightViewPos = mul(output.lightViewPos, lightViewMatrix);
    output.lightViewPos = mul(output.lightViewPos, lightProjectionMatrix);
    
    // Repeat the texture
    output.tex.x = input.tex.x * 2.5f;
    output.tex.y = input.tex.y * 2.5f;

    return output;
}