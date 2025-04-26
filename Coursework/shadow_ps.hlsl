Texture2D shaderTexture : register(t0);
SamplerState diffuseSampler : register(s0);

Texture2D depthMapTexture : register(t1);
SamplerState shadowSampler : register(s1);

cbuffer LightBuffer : register(b0)
{
    float4 ambientColour;
    float4 diffuseColour;

    // Point Light 1
    float3 pointLight1Pos;
    float pointLight1Radius;
    float4 pointLight1Colour;

    // Point Light 2
    float3 pointLight2Pos;
    float pointLight2Radius;
    float4 pointLight2Colour;

    // Directional Light - Moon
    float3 directionalLightDir;
    float timeVal;
    float4 directionalLightColour;

    // Specular Properties
    float specularPower;
    float3 moonPos;
    float4 specularColour;
};

struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPosition : TEXCOORD1;
    float4 lightViewPos : TEXCOORD2;
};

// Helper: Calculate lighting from a point light
float4 CalculatePointLight(float3 lightPos, float pointRadius, float3 worldPos, float3 normal, float4 lightColour)
{
    float3 lightDir = lightPos - worldPos;
    float distance = length(lightDir);
    float attenuation = saturate(1.0f / (distance / pointRadius + 0.1f));
    lightDir = normalize(lightDir);
    float diffuseIntensity = max(dot(normal, lightDir), 0.0f);
    return diffuseIntensity * lightColour * attenuation;
}

// Helper: Calculate lighting from the directional light
float4 CalculateDirectionalLight(float3 lightDir, float3 normal, float4 lightColour)
{
    float diffuseIntensity = max(dot(normal, -lightDir), 0.0f);
    return diffuseIntensity * lightColour;
}

// Helper: Calculate specular highlights
float4 CalculateSpecularLight(float3 normal, float3 viewDir, float3 lightDir, float specularPower, float4 specularColour)
{
    float3 reflectDir = reflect(-lightDir, normal);
    float specAngle = max(dot(viewDir, reflectDir), 0.0f);
    float specular = pow(specAngle, specularPower);
    return specular * specularColour;
}

// Shadow check
bool IsInShadow(Texture2D shadowMap, float4 lightViewPos, float shadowBias)
{
    float2 projCoords = lightViewPos.xy / lightViewPos.w * 0.5f + 0.5f;
    if (projCoords.x < 0.0f || projCoords.x > 1.0f || projCoords.y < 0.0f || projCoords.y > 1.0f)
        return false; // Out of bounds

    float depthValue = shadowMap.Sample(shadowSampler, projCoords).r;
    float lightDepth = lightViewPos.z / lightViewPos.w - shadowBias;

    return lightDepth > depthValue;
}

float4 main(InputType input) : SV_TARGET
{
    // Base texture color
    float4 textureColour = shaderTexture.Sample(diffuseSampler, input.tex);

    // Normalize normal
    float3 normal = normalize(input.normal);

    // Ambient lighting
    float4 ambient = ambientColour;

    // Dynamic directional light calculations
    float3 lightDir = normalize((moonPos + directionalLightDir * 100.0f) - input.worldPosition);
    float4 directional = CalculateDirectionalLight(lightDir, normal, directionalLightColour);

    // Dim the light if in shadow
    if (IsInShadow(depthMapTexture, input.lightViewPos, 0.005f))
    {
        directional *= 0.2f;
    }

    // Point light calculations
    float4 point1 = CalculatePointLight(pointLight1Pos, pointLight1Radius, input.worldPosition, normal, pointLight1Colour);
    float4 point2 = CalculatePointLight(pointLight2Pos, pointLight2Radius, input.worldPosition, normal, pointLight2Colour);

    // Specular light
    float3 viewDir = normalize(-input.worldPosition); // Assuming camera at origin
    float4 specular = CalculateSpecularLight(normal, viewDir, lightDir, specularPower, specularColour);

    // Combine all light contributions
    float4 finalColour = ambient + directional + point1 + point2 + specular;

    // Modulate with texture
    finalColour *= textureColour;

    return saturate(finalColour);
}