/** Terrain Depth Map: Domain Shader Code **/

/****************************************************************************************************************************/

// Textures and samplers for different types of data
Texture2D terrainTexture : register(t0); // Texture for terrain
SamplerState terrainSample : register(s0); // Sampler for terrain texture

Texture2D soilHeightTexture : register(t1); // Soil Height texture
SamplerState soilHeightSample : register(s1); // Sampler for soil height texture

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

// Function to retrieve height data for soil details
float GetSoilHeight(float2 uv)
{
    // Sample the red channel of the soil height map and scale it
    return soilHeightTexture.SampleLevel(soilHeightSample, uv, 0).r * 5.0f;
}

/****************************************************************************************************************************/

[domain("tri")]
OutputType main(ConstantOutputType input, float3 uvCoord : SV_DomainLocation, const OutputPatch<InputType, 3> patch)
{
    OutputType output;
 
    // Compute the vertex position by interpolating the positions of the three control points based on the UV coordinates
    float3 vertexPosition = uvCoord.x * patch[0].position + uvCoord.y * patch[1].position + uvCoord.z * patch[2].position;
    
    // Interpolate the texture coordinates for the patch
    float2 texCoords = uvCoord.x * patch[0].tex + uvCoord.y * patch[1].tex + uvCoord.z * patch[2].tex;
    
    // Adjust terrain hovering height
    //vertexPosition.y -= 5.5f;
    
    // Adjust heigh based on terrain and soil height maps
    //vertexPosition.y += GetTerrainHeight(texCoords) + GetSoilHeight(texCoords);
    
    // World space transformation (to clip space)
    float4 worldPos = mul(float4(vertexPosition, 1.0f), worldMatrix);
    float4 viewPos = mul(worldPos, viewMatrix);
    float4 projPos = mul(viewPos, projectionMatrix);

    output.position = projPos;
    
    output.depthPosition = output.position;
   

    return output;
}