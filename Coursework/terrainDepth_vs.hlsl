/** Tessellation Depth Map: Vertex Shader Code **/

/****************************************************************************************************************************/

// Constant buffer for storing transformation matrices
cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix; // world space
    matrix viewMatrix; // camera view space
    matrix projectionMatrix; // view space coordinates to screen space
};

/****************************************************************************************************************************/

struct InputType
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

/****************************************************************************************************************************/

struct OutputType
{
    float4 position : SV_POSITION;
    float4 depthPosition : TEXCOORD0;
    float2 tex : TEXCOORD1;
};

/****************************************************************************************************************************/

// Main vertex shader function
OutputType main(InputType input)
{
    OutputType output;

    output.position = mul(input.position, worldMatrix); // Transform position to world space
    output.position = mul(output.position, viewMatrix); // to camera view space
    output.position = mul(output.position, projectionMatrix); // to screen space

    output.depthPosition = output.position; // Store the position value in a second input value for depth value calculations.
	
    return output;
}