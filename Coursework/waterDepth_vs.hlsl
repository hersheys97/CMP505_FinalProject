/** Water Depth Map: Vertex Shader Code **/

/****************************************************************************************************************************/

// Constant buffer for storing transformation matrices
cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix; // world space
    matrix viewMatrix; // camera view space
    matrix projectionMatrix; // view space coordinates to screen space
};

// Constant buffer for storing time information
cbuffer TimeBufferType : register(b2)
{
    float time; // Current time elapsed in seconds
    float amplitude; // Height
    float frequency; // wavelength
    float speed; // speed
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
};

/****************************************************************************************************************************/

// Main vertex shader function
OutputType main(InputType input)
{
    OutputType output;

    // Original normal before wave modification
    float3 originalNormal = input.normal;

    // Wave transformation
    float distance = speed * time;
    float x_wave = amplitude * sin(frequency * input.position.x + distance);
    float z_wave = amplitude * cos(frequency * input.position.z + distance);
    input.position.y += x_wave + z_wave;

    // Adjusted normal based on wave calculations
    float dx = amplitude * frequency * cos(frequency * input.position.x + distance);
    float dz = -amplitude * frequency * sin(frequency * input.position.z + distance);
    float3 modifiedNormal = normalize(float3(-dx, 1.0f, -dz));

    // Transform position to world space
    float4 worldPosition = mul(input.position, worldMatrix);

    // Transform position to clip space (for rendering and depth)
    float4 clipPosition = mul(worldPosition, viewMatrix);
    clipPosition = mul(clipPosition, projectionMatrix);

    // Pass the transformed position to depthPosition for depth calculations
    output.depthPosition = clipPosition;
    
    // Pass transformed position to SV_POSITION for rendering
    output.position = clipPosition;
	
    return output;
}