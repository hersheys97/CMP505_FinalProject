/** Bloom: Pixel Shader Code **/

/****************************************************************************************************************************/

// Textures and samplers for different types of data

Texture2D skyDomeTexture : register(t0); // Sky dome texture for the bloom effect
SamplerState skyDomeSampler : register(s0); // Sampler for sky dome texture

/****************************************************************************************************************************/

// Constant buffer to store bloom settings and time
cbuffer ScreenSizeBuffer : register(b0)
{
    float blurAmount; // Controls the strength of the blur applied during the bloom process
    float bloomIntensity; // How bright the bloom effect should appear
    
    float screenWidth; // Width of the screen, used for calculating texel size
    float screenHeight; // Height of the screen, used for calculating texel size
    
    float time; // Current time elapsed in seconds
    float3 padding; // Padding for alignment
};

// Struct to define the input to the pixel shader
struct InputType
{
    float4 position : SV_POSITION; // Pixel position in scren space
    float2 tex : TEXCOORD0; // Texture coordinates for sampling the sky dome texture
};

/****************************************************************************************************************************/

// This function will compute the colour of stars based on the time (loop)
float4 GetStarColour(float time)
{
    // These colours will be used for transitioning the star colours
    float4 white = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float4 lightBlue = float4(0.4f, 0.7f, 1.0f, 1.0f);
    float4 cyan = float4(0.6f, 1.0f, 1.0f, 1.0f);
    float4 yellow = float4(1.0f, 0.9f, 0.3f, 1.0f);

    // Loops from 0 to 4, allowing to apply a colour based on current mod value
    float cycleTime = fmod(time * 0.1f, 4.0f); // Multiplied by 0.1 to slow down the transition

    // Transition between colours based on the current mod value or phase of the cycle
    /* Breakdown:
     * cos(cycleTime * 3.14f) --> This creates a [-1, 1] range in which the 2 colours will be lerped
     * 1 - abc  --> Subtract 1 from the range to make it 1-(-1) = 2 and 1-1 = 0. So, [0, 2]
     * (* 0.5f)  --> To create a normalized range of [0, 1]
     * cycleTime - x.0f  --> To make sure that the range stays normalized between [0, 1] even if the time increases
    */ 
    if (cycleTime < 1.0f)
    {
        // Smooth transition from white to sky blue colour
        return lerp(white, lightBlue, (1.0f - cos(cycleTime * 3.14f)) * 0.5f); // Transition from white to sky blue
    }
    else if (cycleTime < 2.0f)
    {
        return lerp(lightBlue, cyan, (1.0f - cos((cycleTime - 1.0f) * 3.14f)) * 0.5f); // to cyan
    }
    else if (cycleTime < 3.0f)
    {
        return lerp(cyan, yellow, (1.0f - cos((cycleTime - 2.0f) * 3.14f)) * 0.5f); // to yellow
    }
    else
    {
        return lerp(yellow, white, (1.0f - cos((cycleTime - 3.0f) * 3.14f)) * 0.5f); // back to white
    }
}

/****************************************************************************************************************************/

// Main pixel shader function
float4 main(InputType input) : SV_TARGET
{
    // Sample the sky dome texture
    float4 originalColour = skyDomeTexture.Sample(skyDomeSampler, input.tex);
    originalColour *= bloomIntensity; // Apply the bloom intensity

    /****************************************************************************************************************************/
    
    // Bloom is applied to enhance the glow of the stars, simulating light scatter. A brightness threshold determines which pixels contribute to the bloom effect, with a soft threshold for smooth transitions. The bloom factor is calculated using the brightest color channel of each pixel, ensuring only the bright areas contribute to the glow.
    
    // Define thresholds for bloom calculation
    float threshold = 0.04f; // Minimum brightness to start contributing to bloom. Less treshold = more stars
    float softThreshold = 0.2f; // Range for soft transition into bloom
    
    float intensity = max(max(originalColour.r, originalColour.g), originalColour.b); // Get the brightest channel of the colour to bring out the best

    // Calculate the bloom factor, applying a smooth transition for values above the threshold
    float bloomFactor = saturate((intensity - threshold) / softThreshold);

    /****************************************************************************************************************************/
    
    // Screen texel size
    float2 texelSize = float2(1.0f / screenWidth, 1.0f / screenHeight);

    // Next, a Gaussian blur is applied to soften the bloom. The blur kernel is defined with weights, where the central pixel has the highest influence. The blur intensity dynamically adjusts based on the amount of blur needed. 
    
    // Blur weights and sample count for Gaussian blur
    
    int sampleCount = 5;
    float weights[5] = { 0.06136f, 0.24477f, 0.38774f, 0.24477f, 0.06136f };

    // Adjust the sample count based on the blur amount
    if (blurAmount < 1.0f)
    {
        sampleCount = 3; // Fewer samples for low blur amounts
    }
    else if (blurAmount > 1.5f)
    {
        sampleCount = 7; // More samples for high blur amounts
    }

    /****************************************************************************************************************************/
    
    // Initialize the blurred colour - white
    float4 blurredColour = float4(0.0f, 0.0f, 0.0f, 1.0f);
    
    // Neighboring texels are sampled horizontally, and their colors are weighted and combined using Gaussian distribution.

    // Apply Gaussian blur if bloom factor is above 0
    if (bloomFactor > 0.0f)
    {
        // Sample offsets
        for (int i = -2; i <= 2; ++i)
        {
            // Texture coordinate offset
            float2 offset = float2(i, 0) * texelSize * blurAmount;

            // Sample the texture and keep adding the weighted result
            blurredColour += skyDomeTexture.Sample(skyDomeSampler, input.tex + offset) * weights[abs(i)];
        }

        // The final bloom effect is scaled by intensity and factor to control its brightness and glow.
        
        // Scale colour based on intensity and bloom factor
        blurredColour *= bloomIntensity * bloomFactor;
    }

    /****************************************************************************************************************************/
    
    // A dynamic color effect is added to the stars, changing over time.
    // Compute the dynamic star colour based on the time
    float4 starColour = GetStarColour(time);

    //  The final output blends the original color with the bloom effect and star glow.
    // Combine the original colour, star effect, and blurred bloom colour
    float4 finalColour = originalColour * starColour + blurredColour;

    //Apply gamma correction
    //finalColour = pow(finalColour, 1.0f / 2.2f);
    
    // Set the alpha channel of the final colour to full opacity
    finalColour.a = 1.0f;

    // Return the final processed colour
    return finalColour; // pow(finalColour, 2.2f)
}