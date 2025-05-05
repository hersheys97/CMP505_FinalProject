/** Terrain: Hull Shader Code **/
// In the hull shader, tessellation uses triangle patches with three control points. The level of detail is adjusted dynamically based on the camera's distance from the terrain. Closer patches have higher tessellation factors, while distant patches have lower factors, with smooth interpolation between the minimum and maximum values. Integer partitioning ensures uniform tessellation.
/****************************************************************************************************************************/

// Constant buffer for camera position
cbuffer CameraBuffer : register(b0)
{
    float3 cameraPosition; // Camera position in world space
    float padding; // Padding for alignment
};

cbuffer MatrixBuffer : register(b1)
{
    matrix worldMatrix; // world space
    matrix viewMatrix; // camera view space
    matrix projectionMatrix; // view space coordinates to screen space
    matrix lightViewMatrix; // Spotlight - light's view space (for shadow mapping)
    matrix lightProjectionMatrix; // light view space positions to screen space (for shadow mapping)
    matrix lightViewMatrix2; // Directional Light - light's view space (for shadow mapping)
    matrix lightProjectionMatrix2; // light view space positions to screen space (for shadow mapping)
};

/****************************************************************************************************************************/

// Struct to define the input to the hull shader
struct InputType
{
    float3 position : POSITION; // Vertex position in world space
    float2 tex : TEXCOORD0; // Texture coordinates
    float3 normal : NORMAL; // Vertex normal for lighting and tessellation
};

// Struct to define the tessellation factors for each patch edges and the inside
struct ConstantOutputType
{
    float edges[3] : SV_TessFactor; // patch edges
    float inside : SV_InsideTessFactor; // patch interior
};

// Struct to define the output of the tessellator to the domain shader
struct OutputType
{
    float3 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

/****************************************************************************************************************************/

float CalculateEdgeTessFactor(float3 edgeStart, float3 edgeEnd, float3 cameraPos)
{
    float3 edgeCenter = (edgeStart + edgeEnd) * 0.5f;
    float distance = length(cameraPos - edgeCenter);
    
    const float maxD = 5.f;
    const float minD = 15.0f;
    const float maxTess = 64.f;
    const float minTess = 1.f;
    
    return clamp(maxTess * saturate((minD - distance) / (minD - maxD)), minTess, maxTess);
}

// Calculate tessellation factors based on camera distance and patch size
ConstantOutputType PatchConstantFunction(InputPatch<InputType, 3> inputPatch, uint patchId : SV_PrimitiveID)
{
    ConstantOutputType output;

    // Transform control points to world space
    float3 worldPos0 = mul(float4(inputPatch[0].position, 1.0), worldMatrix).xyz;
    float3 worldPos1 = mul(float4(inputPatch[1].position, 1.0), worldMatrix).xyz;
    float3 worldPos2 = mul(float4(inputPatch[2].position, 1.0), worldMatrix).xyz;
    
    // Dynamic tessellation calculation: Detail of the terrain is adjusted based on how far each patch position is from the camera. We are finding the center of a triangle patch in the world and measuring its distance from the camera. If the patch is close, it adds more detail by using a high tessellation factor. If the patch is far, it reduces the detail with a lower tessellation factor. The tessellation factor is calculated smoothly between a maximum and minimum value, making the terrain look detailed when we are close to it while improving performance for far away areas. The factor is applied to all edges and the inside of the triangle patch.
    
    // Compute the center of the patch in world space (average of the three vertices)
    float3 center = (worldPos0 + worldPos1 + worldPos2) / 3.0f;

    // Compute the distance from the camera to the center of the patch
    float distanceToCenter = length(cameraPosition - center);

    // Define the distance interval for tessellation
    const float maxD = 5.f; // Near distance for maximum tessellation
    const float minD = 15.0f; // Far distance for minimum tessellation
    const float maxTess = 64.f;
    const float minTess = 1.f;
    
    // Interpolate the tessellation factor based on distance using saturate function
    float tessFactor = maxTess * saturate((minD - distanceToCenter) / (minD - maxD));
    tessFactor = clamp(tessFactor, minTess, maxTess);

    // Ensure tessellation factor is within [1, 32] (kept small value since my terrain is of low quality)
    tessFactor = max(minTess, tessFactor);
    tessFactor = min(tessFactor, maxTess);
    
    /****************************************************************************************************************************/
    
    // Assign the tessellation factor to all edges and the interior of the patch
    
    // Fix the "gaps"/cracks where the edges meet. Calculate unique edge factors for each edge.
    output.edges[0] = CalculateEdgeTessFactor(worldPos0, worldPos1, cameraPosition);
    output.edges[1] = CalculateEdgeTessFactor(worldPos1, worldPos2, cameraPosition);
    output.edges[2] = CalculateEdgeTessFactor(worldPos2, worldPos0, cameraPosition);

    // Average edge factors for the interior
    output.inside = (output.edges[0] + output.edges[1] + output.edges[2]) / 3.0f;

    return output;
}

/****************************************************************************************************************************/

[domain("tri")]
[partitioning("fractional_odd")] // Partitioning (integer, pow2, fractional_even, fractional_odd)
[outputtopology("triangle_ccw")] // Output triangles in counter-clockwise order
[outputcontrolpoints(3)] // Number of control points per patch. Since using triangle, 3.
[patchconstantfunc("PatchConstantFunction")] // PatchConstantFunction function reference
OutputType main(InputPatch<InputType, 3> patch, uint pointId : SV_OutputControlPointID, uint patchId : SV_PrimitiveID)
{
    OutputType output;

    // Set the position, texture coordinates, and normal for this control point as the output
    output.position = patch[pointId].position;
    output.tex = patch[pointId].tex;
    output.normal = patch[pointId].normal;

    return output;
}