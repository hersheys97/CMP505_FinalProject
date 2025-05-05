/** Terrain Depth Map: Domain Shader Code **/

/****************************************************************************************************************************/

// Textures and samplers for different types of data
Texture2D terrainTexture : register(t0); // Texture for terrain
SamplerState terrainSample : register(s0); // Sampler for terrain texture

/****************************************************************************************************************************/

// Constant buffer for storing transformation matrices
cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix; // world space
    matrix viewMatrix; // camera view space
    matrix projectionMatrix; // view space coordinates to screen space
};

// Struct to define the tessellation factors for each patch edges and the inside
struct ConstantOutputType
{
    float edges[3] : SV_TessFactor;
    float inside : SV_InsideTessFactor;
};

// Struct to define the input to the hull shader
struct InputType
{
    float4 position : SV_POSITION;
    float4 depthPosition : TEXCOORD0;
    float2 tex : TEXCOORD1;
};

// Struct to define the output of the tessellator to the domain shader
struct OutputType
{
    float4 position : SV_POSITION;
    float4 depthPosition : TEXCOORD0;
};

/****************************************************************************************************************************/

// Function to retrieve height data from the terrain height map
float GetTerrainHeight(float2 uv)
{
    // Sample the red channel of the height map and scale the value
    return terrainTexture.SampleLevel(terrainSample, uv, 0).r * 20.0f;
}
/****************************************************************************************************************************/

[domain("tri")]
OutputType main(ConstantOutputType input, float3 bary : SV_DomainLocation, const OutputPatch<InputType, 3> patch)
{
    OutputType output;
 
    // --- Barycentric Interpolation ---
    float3 vertexPosition = bary.x * patch[0].position + bary.y * patch[1].position + bary.z * patch[2].position;
    float2 texCoords = bary.x * patch[0].tex + bary.y * patch[1].tex + bary.z * patch[2].tex;
    
    // Enhanced displacement with scaling
    float height = terrainTexture.SampleLevel(terrainSample, texCoords, 0).r;
    vertexPosition.y += height * 0.2f;
    
    // World space transformation (to clip space)
    float4 worldPos = mul(float4(vertexPosition, 1.0f), worldMatrix);
    float4 viewPos = mul(worldPos, viewMatrix);
    float4 projPos = mul(viewPos, projectionMatrix);

    output.position = projPos;
    output.depthPosition = output.position;
   
    return output;
}