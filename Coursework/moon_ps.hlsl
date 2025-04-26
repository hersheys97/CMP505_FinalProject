/** Moon: Pixel Shader Code **/

/****************************************************************************************************************************/

// Textures and samplers for different types of data
Texture2D moonTexture : register(t0); // Moon texture
SamplerState moonSampler : register(s0); // Sampler for moon texture

/****************************************************************************************************************************/

// Constant buffer to store lighting information
cbuffer LightBuffer : register(b0)
{
    float4 ambientColour; // Ambient light color
    float4 diffuseColour; // Diffuse light color
    float3 moonPos; // Spotlight position (moon)
    float padding; // Padding for alignment
    float4 spotlightColour; // Spotlight color (moonlight)
};

/****************************************************************************************************************************/

// Struct to define the input to the pixel shader
struct InputType
{
    float4 position : SV_POSITION; // Screen-space position
    float2 tex : TEXCOORD0; // Texture coordinates
    float3 normal : NORMAL; // Surface normal
};

/****************************************************************************************************************************/

// Main Shader Function
float4 main(InputType input) : SV_TARGET
{
    float4 textureColour = moonTexture.Sample(moonSampler, input.tex);
    
    // Apply moon colour based on the spotlight colour --> Reduce the intensity.
    float4 moonColour = float4(spotlightColour.xyz * 0.3, 1.f);
    
    // clamp [0, 1]
    float4 finalColour = saturate(textureColour * diffuseColour + moonColour);
    
    //Apply gamma correction
    //finalColour = pow(finalColour, 1.0f / 2.2f);
    
    // Transparency factor
    //finalColour.w = 1.f;
    
    return finalColour; //pow(finalColour, 2.2f)
}