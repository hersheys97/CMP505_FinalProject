/** Firefly: Vertex Shader Code **/

/****************************************************************************************************************************/

// Constant buffer for storing transformation matrices
cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix; // world space
    matrix viewMatrix; // camera view space
    matrix projectionMatrix; // view space coordinates to screen space
};

// Constant buffer for storing camera information
cbuffer CameraBuffer : register(b1)
{
    float3 cameraPosition; // Position of the camera
    float padding; // Padding for alignment
};

/****************************************************************************************************************************/

// Struct to define the input to the vertex shader
struct InputType
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

/****************************************************************************************************************************/

// Struct to define the output to the vertex shader
struct OutputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPosition : TEXCOORD1;
    float3 viewVector : TEXCOORD2;
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
    
    output.normal = mul(input.normal, (float3x3) worldMatrix); // Transform using world matrix without any translation
    output.normal = normalize(output.normal); // normalize
    
    // Vertex shader in world space
    output.worldPosition = mul(input.position, worldMatrix).xyz;
    
    // Vector from vertex to the camera normalized
    float4 worldPosition = mul(input.position, worldMatrix);
    output.viewVector = cameraPosition.xyz - worldPosition.xyz;
    output.viewVector = normalize(output.viewVector);

    return output;
}