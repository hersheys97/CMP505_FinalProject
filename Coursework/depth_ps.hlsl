/** Depth Map: Pixel Shader Code **/

/****************************************************************************************************************************/

// Struct to define the input to the pixel shader
struct InputType
{
    float4 position : SV_POSITION; // Vertex position in clip space
    float4 depthPosition : TEXCOORD0; // Position for depth value calculations
};

/****************************************************************************************************************************/

// Main Shader Function
float4 main(InputType input) : SV_TARGET
{
    float depthValue;
    
	// Get the depth value of the pixel by dividing the Z pixel depth by the homogeneous W coordinate.
    depthValue = input.depthPosition.z / input.depthPosition.w;
    
    float4 finalColour = float4(depthValue, depthValue, depthValue, 1.0f);
    
    return finalColour;
}