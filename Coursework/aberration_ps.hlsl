Texture2D sceneTexture : register(t0);
SamplerState sampleState : register(s0);

cbuffer AberrationBuffer : register(b1)
{
    float2 chromaticOffset;
    float2 ghostScreenPos; // Normalized screen position (0-1)
    float ghostDistance; // 0-1 normalized distance (1 = closest)
    float time; // For animated effects
    float effectIntensity; // Overall strength (0-1)
    float padding;
};

struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD;
};

// Improved noise functions
float hash(float2 p)
{
    p = float2(dot(p, float2(127.1, 311.7)),
              dot(p, float2(269.5, 183.3)));
    return -1.0 + 2.0 * frac(sin(p) * 43758.5453123);
}

float noise(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    
    float2 u = f * f * (3.0 - 2.0 * f);
    
    return lerp(lerp(hash(i + float2(0.0, 0.0)),
                     hash(i + float2(1.0, 0.0)), u.x),
                lerp(hash(i + float2(0.0, 1.0)),
                     hash(i + float2(1.0, 1.0)), u.x), u.y);
}

float4 main(InputType input) : SV_TARGET
{
    float2 uv = input.tex;
    
    // Calculate distance from ghost screen position
    float2 ghostVec = uv - ghostScreenPos;
    float ghostDist = length(ghostVec);
    float proximityFactor = saturate(1.0 - ghostDist * 2.0); // Stronger near ghost
    
    // Base chromatic aberration with distance scaling
    float2 offset = chromaticOffset * ghostDistance * effectIntensity;
    
    // Radial distortion toward ghost
    float2 ghostDirection = normalize(ghostVec);
    float2 radialOffset = ghostDirection * 0.05 * proximityFactor * ghostDistance;
    
    // Combined offsets
    float2 finalOffset = offset + radialOffset;
    
    // Time-based pulsing
    float pulse = 0.5 + 0.5 * sin(time * 3.0);
    float distortionWave = sin(time * 5.0 + ghostDist * 20.0) * 0.1 * ghostDistance;
    
    // Channel separation with distortion
    float r = sin((uv.x + finalOffset.x * 1.5 + distortionWave) * 100.0) * 0.5 + 0.5;
    float g = cos((uv.y + finalOffset.y * 0.9 - distortionWave) * 110.0) * 0.5 + 0.5;
    float b = sin((uv.x - uv.y + distortionWave * 2.0) * 120.0) * 0.5 + 0.5;
    
    // Proximity-based effects
    float staticNoise = noise(uv * 50.0 + time) * ghostDistance;
    float glow = saturate(0.5 - ghostDist) * ghostDistance;
    
    // Combine effects
    r = saturate(r + staticNoise * 0.3 + glow * 0.5);
    g = saturate(g + staticNoise * 0.2 + glow * 0.3);
    b = saturate(b + staticNoise * 0.4 + glow * 0.2);
    
    // Ghost "breathing" effect
    float breathing = sin(time * 1.5) * 0.1 * ghostDistance;
    float2 breathingOffset = ghostDirection * breathing;
    
    // Final color with alpha
    float4 color = float4(r, g, b, 0.1 + ghostDistance * 0.3);
    
    // Radial vignette near ghost
    float vignette = 1.0 - smoothstep(0.3, 0.7, ghostDist) * ghostDistance;
    color.rgb *= vignette;
    color.w = 0.0001f;
    
    return color;
}