/** Water: Vertex Shader Code **/

/****************************************************************************************************************************/

// Constant buffer for storing transformation matrices
cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix; // world space
    matrix viewMatrix; // camera view space
    matrix projectionMatrix; // view space coordinates to screen space
    matrix lightViewMatrix; // Spotlight - light's view space (for shadow mapping)
    matrix lightProjectionMatrix; // light view space positions to screen space (for shadow mapping)
    matrix lightViewMatrix2; // Directional Light - light's view space (for shadow mapping)
    matrix lightProjectionMatrix2; // light view space positions to screen space (for shadow mapping)
};

// Constant buffer for storing camera information
cbuffer CameraBuffer : register(b1)
{
    float3 cameraPosition; // Position of the camera
    float padding;
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
    float3 originalNormal : TEXCOORD1;
    float3 worldPosition : TEXCOORD2;
    float4 lightViewPos : TEXCOORD3;
    float3 viewVector : TEXCOORD4;
    float4 lightViewPos2 : TEXCOORD5;
};

/****************************************************************************************************************************/

// Main vertex shader function
OutputType main(InputType input)
{
    OutputType output;
    
    // Original normal before wave modification
    float3 originalNormal = input.normal;
    
    // Water wave motion is created using sine and cosine functions to displace the y-position of vertices. Normals are recalculated based on derivatives to ensure proper lighting. A static normal (originalNormal) is maintained for specular lighting, keeping reflections stationary.
    // Wave transformation
    float distance = speed * time;
    float x_wave = amplitude * sin(frequency * input.position.x + distance);
    float z_wave = amplitude * cos(frequency * input.position.z + distance);
    input.position.y += x_wave + z_wave;

    // Adjusted normal based on wave calculations (derivatives)
    float dx = amplitude * frequency * cos(frequency * input.position.x + distance);
    float dz = -amplitude * frequency * sin(frequency * input.position.z + distance);
    float3 modifiedNormal = normalize(float3(-dx, 1.0f, -dz));

    
    output.position = mul(input.position, worldMatrix); // Transform position to world space
    output.position = mul(output.position, viewMatrix); // to camera view space
    output.position = mul(output.position, projectionMatrix); // to screen space
    
    // Calculate the position of the vertice as viewed by the light source.
    output.lightViewPos = mul(input.position, worldMatrix);
    output.lightViewPos = mul(output.lightViewPos, lightViewMatrix);
    output.lightViewPos = mul(output.lightViewPos, lightProjectionMatrix);
    
    output.lightViewPos2 = mul(input.position, worldMatrix);
    output.lightViewPos2 = mul(output.lightViewPos2, lightViewMatrix2);
    output.lightViewPos2 = mul(output.lightViewPos2, lightProjectionMatrix2);
    

    // Repeat texture 
    output.tex.x = input.tex.x * 4.0f;
    output.tex.y = input.tex.y * 4.0f;

    output.normal = normalize(mul(modifiedNormal, (float3x3) worldMatrix)); // Transform using world matrix without any translation
    output.originalNormal = normalize(mul(originalNormal, (float3x3) worldMatrix)); // Unmodified normal

    // World space
    output.worldPosition = mul(input.position, worldMatrix).xyz;
    
    // Vector from vertex to the camera normalized
    float4 worldPosition = mul(input.position, worldMatrix);
    output.viewVector = cameraPosition.xyz - worldPosition.xyz;
    output.viewVector = normalize(output.viewVector);

    return output;
}