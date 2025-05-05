/** Terrain: Pixel Shader Code **/

/****************************************************************************************************************************/

// Textures and samplers for different types of data
Texture2D terrainTexture : register(t0); // Height map for the terrain
SamplerState terrainSampler : register(s0); // Sampler for the terrain height map

Texture2D depthMapSpotlight : register(t1); // Shadow depth map texture - spotlight 
SamplerState sampleSpotlight : register(s1); // Sampler for the depth map

Texture2D depthMapDirectional : register(t2); // Shadow depth map texture - Directional light
SamplerState sampleDirectional : register(s2); // Sampler for the depth map

/****************************************************************************************************************************/

// Constant buffer to store lighting information and material properties
cbuffer LightBuffer : register(b0)
{
    float4 ambientColour; // Ambient light colour
    float4 diffuseColour; // Diffuse light colour

    // Point light 1 properties
    float3 pointLight1Pos; // Position of point light 1
    float pointLight1Radius; // Radius of point light 1's influence
    float4 pointLight1Colour; // Colour of point light 1

    // Point light 2 properties
    float3 pointLight2Pos; // Position of point light 2
    float pointLight2Radius; // Radius of point light 2's influence
    float4 pointLight2Colour; // Colour of point light 2

    // Spotlight (moonlight) properties
    float4 spotlightColour; // Colour of the spotlight
    float spotlightCutoff; // Cutoff angle for spotlight cone
    float3 spotlightDirection; // Direction of the spotlight
    float spotlightFalloff; // Falloff rate of spotlight intensity
    float3 moonPos; // Position of the spotlight source

    // Specular lighting properties
    float4 specularColour; // Specular highlight colour
    float specularPower; // Specular intensity control
    
    float3 directionalLightDirection; // Directional light direction
    float4 directionalColour; // directional light colour
};

cbuffer SonarBuffer : register(b1)
{
    float3 sonarOrigin;
    float sonarRadius;
    bool sonarActive;
    float sonarTime;
    float sonarDuration;
    float padding;
};

/****************************************************************************************************************************/

// Struct to define the input to the pixel shader
struct InputType
{
    float4 position : SV_POSITION; // Vertex position in clip space
    float2 tex : TEXCOORD0; // Texture coordinates
    float3 normal : NORMAL; // Surface normal
    float3 worldNormal : TEXCOORD1; // Normal in world space
    float3 worldPosition : TEXCOORD2; // Vertex position in world space
    float4 lightViewPos : TEXCOORD3; // Position in light's view space - Spotlight
    float3 viewVector : TEXCOORD4; // Vector from vertex to camera
    float4 lightViewPos2 : TEXCOORD5; // Position in light's view space - Direcitonal
};

/****************************************************************************************************************************/

// Directional Light: Light intensity is based on the dot product of the surface normal and light direction, clamped between 0 and 1. The directional light color is scaled by this intensity.
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

// Point Lights: Attenuation decreases light intensity as distance increases, calculated using the distance squared from the light source. The final color scales with the attenuation and diffuse intensity.
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

// Spotlight: The spotlight effect is based on the angle between the light direction and spotlight direction. Light diminishes near the edges of the spotlight using a falloff factor, with attenuation based on distance.
// Spotlight calculation
float4 CalculateSpotlight(float3 moonPos, float3 lightDirection, float cutoff, float falloff, float3 worldPos, float3 normal, float4 lightColour)
{
    lightDirection = normalize(lightDirection);

    // Initialize the final spotlight contribution as zero.
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

// Specular Lighting: The halfway vector between the view direction and light direction determines the specular intensity. Sharpness is controlled by the specular power and glossiness factor.
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

// Shadow Mapping
// Shadow Mapping: The function converts light space positions into texture coordinates to sample the shadow map. Depth values are compared to detect whether a fragment is in shadow, with a bias applied to reduce shadow acne.
// Author: Ruth Falconer
// Converts the light's view-space position into texture coordinates for shadow mapping
float2 getProjectiveCoords(float4 lightViewPosition)
{
    float2 projTex = lightViewPosition.xy / lightViewPosition.w; // Perspective divide to normalize coordinates
    projTex = projTex * 0.5f + 0.5f; // normalized coordinates from [-1, 1] to [0, 1]
    return projTex;
}

// Determines if a fragment is in shadow using the shadow map and bias
bool isInShadow(Texture2D shadowMap, float2 uv, float4 lightViewPosition, float bias, int index)
{
    float depthValue;
    if (index = 0)
        depthValue = shadowMap.Sample(sampleSpotlight, uv).r; // Sample the shadow map to get the stored depth value.
    else
        depthValue = shadowMap.Sample(sampleDirectional, uv).r; // Sample the shadow map to get the stored depth value.
    
    float lightDepthValue = lightViewPosition.z / lightViewPosition.w; // Compute the depth of the current fragment in light space.
    lightDepthValue -= bias; // Apply a bias to reduce shadow artifacts (e.g., shadow acne).

    // Compare the fragment's depth with the shadow map's depth, using a small epsilon to avoid hard edges.
    return lightDepthValue > depthValue + 0.001f; // Return true if the fragment is in shadow.
}

/****************************************************************************************************************************/

// Main Shader Function
float4 main(InputType input) : SV_TARGET
{
    // Sample terrain texture and get normal
    float4 terrainColour = terrainTexture.Sample(terrainSampler, input.tex);
    float3 normalizedNormal = normalize(input.normal);
    float3 worldNormal = normalize(input.worldNormal);
    
    /****************************************************************************************************************************/
    
    // Calculate directional light
    float4 directionalLight = CalculateDirectionalLight(worldNormal);
    
    /****************************************************************************************************************************/
    
    // Calculate point lights
    float4 pointLight1 = CalculatePointLight(pointLight1Pos, pointLight1Radius, input.worldPosition, worldNormal, pointLight1Colour);
    float4 pointLight2 = CalculatePointLight(pointLight2Pos, pointLight2Radius, input.worldPosition, worldNormal, pointLight2Colour);
    
    // Point lights intensity
    float intensityFactor = 5.f;
    pointLight1 *= intensityFactor;
    pointLight2 *= intensityFactor;
    
    /****************************************************************************************************************************/
    
    // Calculate spotlight (moonlight)
    float4 spotlight = CalculateSpotlight(moonPos, spotlightDirection, spotlightCutoff, spotlightFalloff, input.worldPosition, worldNormal, spotlightColour);

    // Calculate specular light from the spotlight
    float4 specular = CalculateSpecularLight(normalize(spotlightDirection), worldNormal, normalize(input.viewVector), specularColour, specularPower);
    
    /****************************************************************************************************************************/
    
    // Calculate shadow factor for directional light using shadow mapping.
    float2 shadowUV = getProjectiveCoords(input.lightViewPos); // Convert light-space position to shadow map coordinates
    bool shadowed = isInShadow(depthMapSpotlight, shadowUV, input.lightViewPos, 0.005f, 0); // Check if the fragment is in shadow
    float shadowFactor = shadowed ? 0.005f : 1.0f; // Reduce light intensity if the fragment is in shadow
    
    float2 shadowUV2 = getProjectiveCoords(input.lightViewPos2); // Convert light-space position to shadow map coordinates
    bool shadowed2 = isInShadow(depthMapDirectional, shadowUV2, input.lightViewPos2, 0.005f, 1); // Check if the fragment is in shadow
    float shadowFactor2 = shadowed2 ? 0.005f : 1.0f; // Reduce light intensity if the fragment is in shadow
    
    /****************************************************************************************************************************/
    
    float4 ambient = ambientColour * terrainColour * 0.8f; // Reduced intensity
    float4 finalColour = ambient + (pointLight1 + pointLight2) * 0.9f + (directionalLight * shadowFactor2 * 0.7f) + (spotlight * shadowFactor * 0.8f);
    
    finalColour += specular * 0.6f;
    finalColour = saturate(finalColour);
    
    /****************************************************************************************************************************/
    // Sonar wireframe overlay
    
    // Sonar wireframe overlay
    if (sonarActive)
    {
        float3 toPixel = input.worldPosition - sonarOrigin;
        float dist = length(toPixel);

    // Speed control (now much faster)
        float speedMultiplier = 5.0f;
        float waveProgress = (sonarTime * speedMultiplier) / sonarDuration;

    // 1. Stronger Reveal Area Around Player
        float revealRadius = 60.0f;
        float revealFactor = smoothstep(revealRadius, revealRadius - 5.0f, dist);
        finalColour.rgb += float3(0.1f, 0.3f, 0.4f) * revealFactor * 1.5f;

    // 2. More Visible Expanding Ring
        float ringPosition = waveProgress * sonarRadius;
        float ringWidth = 8.0f; // Wider ring
        float ring = smoothstep(ringPosition - ringWidth, ringPosition, dist) *
                (1.0 - smoothstep(ringPosition, ringPosition + ringWidth, dist));

    // Increased intensity with stronger falloff
        float ringIntensity = 2.0f - smoothstep(0.0f, sonarRadius * 0.7f, dist);
        float3 ringColor = float3(0.3f, 1.0f, 1.0f) * ring * ringIntensity * 4.0f; // Brighter cyan

    // More pronounced pulsing effect
        float pulse = 0.7f + 0.5f * sin(sonarTime * 50.0f); // Faster, stronger pulse
        ringColor *= pulse * 1.5f;

    // Add glow effect around the ring
        float glow = ring * 0.5f;
        ringColor += float3(0.4f, 0.9f, 1.0f) * glow;

    // Combine with final color (stronger additive blending)
        finalColour.rgb += ringColor * 1.5f;

    // 3. More Visible Fading Echo
        if (dist < ringPosition)
        {
            float fade = 1.0f - (ringPosition - dist) / ringPosition;
            float3 echoColor = float3(0.2f, 0.5f, 0.8f) * fade * 0.8f; // Brighter echo
            finalColour.rgb += echoColor;
        }

    // Additional: Add subtle screen-wide pulse when sonar starts
        float globalPulse = saturate(1.0 - sonarTime * 2.0);
        finalColour.rgb += float3(0.1f, 0.3f, 0.5f) * globalPulse * 0.3f;
    }
    //else return float4(1.0f, 1.0f, 1.0f, 1.0f); // Return white if sonar is not active)
    
    //Apply gamma correction
    //finalColour = pow(finalColour, 1.0f / 2.2f);
    
    finalColour.w = 1.f; // Setting alpha
    
    // return finalColour; // pow(finalColour, 2.2f)
    return finalColour;
}