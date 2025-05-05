/** Terrain: Vertex Shader Code **/

/****************************************************************************************************************************/

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

// Struct to define the input to the vertex shader
struct InputType
{
    float3 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

/****************************************************************************************************************************/

// Struct to define the output to the vertex shader
struct OutputType
{
    float3 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

/****************************************************************************************************************************/

// Main vertex shader function
OutputType main(InputType input)
{
    OutputType output;
    
    output.position = input.position;
   // output.tex = input.tex;
    output.tex = input.tex * float2(5.0f, 5.0f);
    output.normal = normalize(input.normal);
    
    // Transform position to world space immediately
   //output.position = mul(float4(input.position, 1.0f), worldMatrix).xyz;
   //output.tex = input.tex;
   //output.normal = normalize(mul(input.normal, (float3x3) worldMatrix));
    
    return output;
}