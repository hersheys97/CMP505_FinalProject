/** Firfly: Pixel Shader Code **/

/****************************************************************************************************************************/

// Textures and samplers for different types of data

Texture2D fireflyTexture : register(t0); // Yellow texture for sphere
SamplerState fireflySampler : register(s0); // Sampler for the yellow texture

/****************************************************************************************************************************/

// Constant buffer to store lighting information and material properties
cbuffer LightBuffer : register(b0)
{
    float4 ambientColour; // Ambient light color
    float4 diffuseColour; // Diffuse light color

    // Point light 1 properties
    float3 pointLight1Pos; // Position of point light 1
    float pointLight1Radius; // Radius of point light 1's influence
    float4 pointLight1Colour; // Color of point light 1

    // Point light 2 properties
    float3 pointLight2Pos; // Position of point light 2
    float pointLight2Radius; // Radius of point light 2's influence
    float4 pointLight2Colour; // Color of point light 2

    // Spotlight (moonlight) properties
    float4 spotlightColour; // Color of the spotlight
    float spotlightCutoff; // Cutoff angle for spotlight cone
    float3 spotlightDirection; // Direction of the spotlight
    float spotlightFalloff; // Falloff rate of spotlight intensity
    float3 moonPos; // Position of the spotlight source

    // Specular lighting properties
    float4 specularColour; // Specular highlight color
    float specularPower; // Specular intensity control
    
    // Time properties
    float timeVal; // Time passed in seconds
    float2 padding; // Extra padding for alignment
    float padding2;
    
    // Directional Light properties
    float3 directionalLightDirection; // Directional light direction
    float4 directionalColour; // directional light colour
};

/****************************************************************************************************************************/

// Struct to define the input to the pixel shader
struct InputType
{
    float4 position : SV_POSITION; // Vertex position in clip space
    float2 tex : TEXCOORD0; // Texture coordinates
    float3 normal : NORMAL; // Surface normal
    float3 worldPosition : TEXCOORD1; // Vertex position in world space
    float3 viewVector : TEXCOORD2; // Vector from vertex to camera
};

/****************************************************************************************************************************/

// This function calculates the contribution of a directional light source.
float4 CalculateDirectionalLight(float3 normal)
{
    // Normalize the light direction vector.
    float3 dir = normalize(directionalLightDirection);

    // Compute the intensity of light based on the angle between the surface normal and light direction.
    float intensity = saturate(dot(normal, -dir)); // Saturate clamps between 0 and 1.

    // Return the light colour scaled by its intensity, blending with ambient and diffuse colours.
    return intensity * directionalColour;
}

/****************************************************************************************************************************/

// For 2 point lights
float4 CalculatePointLight(float3 lightPos, float pointRadius, float3 worldPos, float3 normal, float4 lightColour)
{
    // Compute the vector from the light source to the fragment's world position.
    float3 lightDir = lightPos - worldPos;

    // Calculate the distance from the light source to the fragment.
    float distance = length(lightDir);

    // Apply a threshold to prevent too strong light near the surface.
    float threshold = 2.f; // Define the threshold distance where light stops increasing
    float minDistance = max(distance - pointRadius, threshold); // Ensures minimum distance effect
    
    // Compute attenuation based on distance with a threshold applied.
    float attenuation = 1.0 / (minDistance * minDistance);

    // Normalize the light direction for accurate calculations.
    lightDir = normalize(lightDir);

    // Calculate the diffuse lighting intensity using the surface normal and light direction.
    float diffuse = max(dot(normal, lightDir), 0.0);

    // Return the final point light contribution with attenuation applied.
    return diffuse * lightColour * attenuation;
}

/****************************************************************************************************************************/

// Spotlight calculation
float4 CalculateSpotlight(float3 moonPos, float3 lightDirection, float cutoff, float falloff, float3 worldPos, float3 normal, float4 lightColour)
{
    lightDirection = normalize(lightDirection);

    // Initialize the final spotlight contribution as black.
    float3 finalColor = float3(0.0f, 0.0f, 0.0f);

    // Set spotlight range and brightness factors for its intensity.
    float range = 150.0f; // Effective range of the spotlight.
    float brightnessFactor = 8.0f; // Controls spotlight brightness.

    // Compute the light vector and distance from the light to the fragment.
    float3 lightVector = moonPos - worldPos;
    float distance = length(lightVector);
    float3 lightDir = normalize(lightVector);

    // Calculate the diffuse lighting contribution.
    float diffuseFactor = max(dot(normal, lightDir), 0.0f);

    // Check if the fragment is within the spotlight's range and is lit.
    if (diffuseFactor > 0.0f && distance <= range)
    {
        // Compute the spotlight effect based on the angle between its direction and the light vector.
        float spotEffect = dot(normalize(-lightDirection), lightDir);
        spotEffect = saturate(spotEffect); // Clamp -> [0, 1].

        // Check if the angle is within the spotlight's cutoff.
        if (spotEffect > cutoff)
        {
            // Apply a falloff factor to the spotlight's intensity.
            float angleEffect = pow(spotEffect, falloff);

            // Calculate linear attenuation based on the distance from the spotlight.
            float attenuation = saturate(1.0f - (distance / range));

            // Combine all factors to compute the final spotlight contribution.
            finalColor += lightColour.rgb * diffuseFactor * angleEffect * attenuation * brightnessFactor;
        }
    }

    // Return the spotlight contribution with the original alpha value.
    return float4(saturate(finalColor), lightColour.a);
}

/****************************************************************************************************************************/

// Specular lighting for spotlight
float4 CalculateSpecularLight(float3 lightDirection, float3 normal, float3 viewVector, float4 colour, float power)
{
    lightDirection = normalize(lightDirection);
    viewVector = normalize(viewVector);
    normal = normalize(normal);

    // Calculate the halfway vector between the light direction and the view direction
    float3 halfway = normalize(lightDirection + viewVector);

    // Compute the intensity of the specular reflection.
    float specIntensity = pow(max(dot(normal, halfway), 0.0), power);

    // Glossiness to control the sharpness of the highlight
    float glossiness = 0.8;
    specIntensity *= glossiness;

    // Return the final specular light contribution
    return colour * specIntensity;
}

/****************************************************************************************************************************/

// Main Shader Function
float4 main(InputType input) : SV_TARGET
{
    // Sample the yellow colour
    float4 textureColour = fireflyTexture.Sample(fireflySampler, input.tex);
    
    /****************************************************************************************************************************/
    
    // Calculate directional light
    float4 directionalLight = CalculateDirectionalLight(normalize(input.normal));
    
    /****************************************************************************************************************************/
    
    // Calculate point lights
    float4 pointLight1 = CalculatePointLight(pointLight1Pos, pointLight1Radius, input.worldPosition, normalize(input.normal), pointLight1Colour);
    float4 pointLight2 = CalculatePointLight(pointLight2Pos, pointLight2Radius, input.worldPosition, normalize(input.normal), pointLight2Colour);
    
    // Define an intensity scaling factor for the point lights.
    float intensityFactor = 5.0f;
    pointLight1 *= intensityFactor;
    pointLight2 *= intensityFactor;
    
    /****************************************************************************************************************************/
    
    // Calculate spotlight (moonlight)
    float4 spotlight = CalculateSpotlight(moonPos, spotlightDirection, spotlightCutoff, spotlightFalloff, input.worldPosition, normalize(input.normal), spotlightColour);
    float3 normalizedSpotlightDirection = normalize(spotlightDirection);

    // Calculate specular light from the spotlight
    float4 specular = CalculateSpecularLight(normalizedSpotlightDirection, normalize(input.normal), input.viewVector, specularColour, specularPower);
    
    /****************************************************************************************************************************/
    
    // Pulsating firefly effect - A sine wave, scaled and shifted to range between 0 and 1, controls the intensity of a pulsating effect. This creates a smooth, cyclic visual change over time.
    // A sine function is used to create a pulsating effect based on the time.
    float pulsate = sin(timeVal * 2.0f * 3.14f) * 0.5f + 0.5f; // [0, 1]
    
    /****************************************************************************************************************************/
    
    // Combine all lighting contributions into the final colour.
    float4 finalColour = (ambientColour * textureColour) + pointLight1 + pointLight2 + (directionalLight) + (spotlight);
    // +pointLight1Diffuse + pointLight2Diffuse;
    
    // Add specular highlights
    finalColour += specular;
    
    // Modulate the final color with the pulsating factor
    finalColour *= pulsate;
    
    // Clamp final colour
    finalColour = saturate(finalColour * textureColour);
    
    //Apply gamma correction
    //finalColour = pow(finalColour, 1.0f / 2.2f);
    
    finalColour.w = 1.f; // Setting alpha
    
    return finalColour; //pow(finalColour, 2.2f)
}