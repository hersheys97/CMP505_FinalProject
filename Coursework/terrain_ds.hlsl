/** Terrain: Domain Shader Code **/
// In the domain shader, vertex positions are calculated using UV coordinates to interpolate between control points. Texture coordinates are similarly interpolated for smooth geometry formation.
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
    matrix lightViewMatrix; // Spotlight - light's view space (for shadow mapping)
    matrix lightProjectionMatrix; // light view space positions to screen space (for shadow mapping)
    matrix lightViewMatrix2; // Directional Light - light's view space (for shadow mapping)
    matrix lightProjectionMatrix2; // light view space positions to screen space (for shadow mapping)
};

// Constant buffer for storing camera information
cbuffer CameraBuffer : register(b1)
{
    float3 cameraPosition;
    float padding;
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
    float3 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

// Struct to define the output of the tessellator to the domain shader
struct OutputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldNormal : TEXCOORD1;
    float3 worldPosition : TEXCOORD2;
    float4 lightViewPos : TEXCOORD3; // Light view position for shadow mapping
    float3 viewVector : TEXCOORD4; // Vector from vertex to camera
    float4 lightViewPos2 : TEXCOORD5;
};

/****************************************************************************************************************************/

[domain("tri")]
OutputType main(ConstantOutputType input, float3 bary : SV_DomainLocation, const OutputPatch<InputType, 3> patch)
{
    OutputType output;
    
    // --- Barycentric Interpolation ---
    float3 vertexPosition = bary.x * patch[0].position + bary.y * patch[1].position + bary.z * patch[2].position;
    float2 texCoords = bary.x * patch[0].tex + bary.y * patch[1].tex + bary.z * patch[2].tex;
    float3 normal = bary.x * patch[0].normal + bary.y * patch[1].normal + bary.z * patch[2].normal;

    // Enhanced displacement with scaling
    float height = terrainTexture.SampleLevel(terrainSample, texCoords, 0).r;
    vertexPosition.y += height * 0.2f;
    
    // World space transformation (to clip space)
    float4 worldPos = mul(float4(vertexPosition, 1.0f), worldMatrix);
    float4 viewPos = mul(worldPos, viewMatrix);
    float4 projPos = mul(viewPos, projectionMatrix);

    output.position = projPos;
    output.worldPosition = worldPos.xyz;
    output.worldNormal = normalize(mul(patch[0].normal, (float3x3) worldMatrix));
    output.normal = normalize(normal);

    // Light space transformation - spotlight
    float4 lightPos = mul(worldPos, lightViewMatrix);
    output.lightViewPos = mul(lightPos, lightProjectionMatrix);
    
    // Light space transformation - direcitional light
    float4 lightPos2 = mul(worldPos, lightViewMatrix2);
    output.lightViewPos2 = mul(lightPos2, lightProjectionMatrix2);
    
    // Repeat the texture
    output.tex = texCoords * 3.f;
    
    // View vector for lighting
    output.viewVector = cameraPosition - worldPos.xyz;

    return output;
}