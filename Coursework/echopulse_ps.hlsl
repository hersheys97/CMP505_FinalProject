Texture2D sceneTexture : register(t0);
SamplerState sampleState : register(s0);

cbuffer PulseBuffer : register(b0)
{
    float sonarMaxRadius;
    float sonarTime;
    float screenWidth;
    float screenHeight;
    float3 sonarOrigin;
    bool sonarActive;
};

struct PixelInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

float4 main(PixelInput input) : SV_Target
{
    // 1. Get screen position and distance from origin
    float2 screenPos = input.texCoord * float2(screenWidth, screenHeight);
    float dist = distance(screenPos, sonarOrigin.xy);
    
    // 2. Calculate pulse progress (0-1 over 3 seconds)
    float pulseProgress = saturate(sonarTime / 3.0);
    float pulseRadius = sonarMaxRadius * pulseProgress;
    
    // 3. Create pulse wave effect
    float waveWidth = 15.0; // Width of the pulse wave
    float waveEdge = smoothstep(pulseRadius - waveWidth, pulseRadius, dist);
    float waveCore = smoothstep(pulseRadius - waveWidth * 0.5, pulseRadius, dist);
    
    // 4. Combine colors
    float4 terrainColor = sceneTexture.Sample(sampleState, input.texCoord);
    float4 pulseColor = float4(0.2, 0.8, 1.0, 1.0); // Light blue pulse
    
    // 5. Blend pulse with terrain
    float pulseStrength = (1.0 - waveEdge) * (waveCore);
    float4 finalColor = lerp(terrainColor, pulseColor, pulseStrength);
    
    // 6. Add distance-based fade
    finalColor.a = 1.0 - saturate(dist / sonarMaxRadius);
    
    return finalColor;
}