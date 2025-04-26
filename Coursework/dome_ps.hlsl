/** Dome: Pixel Shader Code **/

/****************************************************************************************************************************/

// Textures and samplers for different types of data
Texture2D renderTex : register(t0); // Render texture with bloom effect
SamplerState renderTexSampler : register(s0); // Sampler for render texture

/****************************************************************************************************************************/

// Struct to define the input to the pixel shader
struct InputType
{
    float4 position : SV_POSITION; // Vertex position (screen space)
    float2 tex : TEXCOORD0; // Texture coordinates
    float3 normal : NORMAL; // Surface normal
};

/****************************************************************************************************************************/

// Main Shader Function
float4 main(InputType input) : SV_TARGET
{
    // Sample the render texture
    float4 textureColor = renderTex.Sample(renderTexSampler, input.tex);
   
    //Apply gamma correction
    //float4 finalColour = pow(textureColor, 1.0f / 2.2f);
    
    // Transparency factor
    textureColor.w = 1.f;
    
    return textureColor; //pow(finalColour, 2.2f)
}