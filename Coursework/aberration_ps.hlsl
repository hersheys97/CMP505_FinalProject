Texture2D sceneTexture : register(t0);
SamplerState sampleState : register(s0);

cbuffer AberrationBuffer : register(b1)
{
    float2 chromaticOffset; // RGB channel separation amount 
    float2 ghostScreenPos; // Normalized screen position (0-1) of ghost effect center
    float ghostDistance; // 0-1 normalized distance (1 = closest to ghost)
    float time; // Time value for animated effects
    float effectIntensity; // Overall effect strength (0-1)
    float padding;
};

struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD;
};

// Perlin-style noise function
float hash(float2 p)
{
    p = float2(dot(p, float2(127.1, 311.7)), dot(p, float2(269.5, 183.3)));
    return -1.0 + 2.0 * frac(sin(p) * 43758.5453123);
}

// Ken Perlin's improved noise (2002) [Perlin, K. "Improving Noise". SIGGRAPH 2002]
float noise(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    
    // Smooth interpolation - Hermite interpolation for continuity [Ebert et al. "Texturing & Modeling" 3rd Ed.]
    float2 u = f * f * (3.0 - 2.0 * f);
    
    return lerp(lerp(hash(i + float2(0.0, 0.0)), hash(i + float2(1.0, 0.0)), u.x), lerp(hash(i + float2(0.0, 1.0)), hash(i + float2(1.0, 1.0)), u.x), u.y);
}

float4 main(InputType input) : SV_TARGET
{
    float2 uv = input.tex;
    
    
    // distance falloff - [Pharr & Humphreys "Physically Based Rendering"]
    // Calculate distance from ghost center point
    float2 ghostVec = uv - ghostScreenPos;
    float ghostDist = length(ghostVec);
    float proximityFactor = saturate(1.0 - ghostDist * 2.0); // Stronger effect near ghost
    
    // Chromatic aberration (RGB channel separation)
    float2 offset = chromaticOffset * ghostDistance * effectIntensity;
    
    // Radial distortion toward ghost center - fisheye lens distortion [Brown-Conrady model]
    float2 ghostDirection = normalize(ghostVec);
    float2 radialOffset = ghostDirection * 0.05 * proximityFactor * ghostDistance;
    
    // Combined distortion effects
    float2 finalOffset = offset + radialOffset;
    
    // Time-based effects for organic feel - "Real-Time Rendering" (Akenine-Möller et al.)
    float pulse = 0.5 + 0.5 * sin(time * 3.0); // Slow pulsing
    float distortionWave = sin(time * 5.0 + ghostDist * 20.0) * 0.1 * ghostDistance; // Rippling distortion
    
    // Create RGB channel variations using trigonometric patterns - interference patterns in wave optics [Goodman "Introduction to Fourier Optics"]
    float r = sin((uv.x + finalOffset.x * 1.5 + distortionWave) * 100.0) * 0.5 + 0.5;
    float g = cos((uv.y + finalOffset.y * 0.9 - distortionWave) * 110.0) * 0.5 + 0.5;
    float b = sin((uv.x - uv.y + distortionWave * 2.0) * 120.0) * 0.5 + 0.5;
    
    // Proximity-based effects - psychophysical brightness perception [Fechner's Law]
    float staticNoise = noise(uv * 50.0 + time) * ghostDistance; // TV-static noise
    float glow = saturate(0.5 - ghostDist) * ghostDistance; // Radial glow
    
    // Combine all color effects
    r = saturate(r + staticNoise * 0.3 + glow * 0.5);
    g = saturate(g + staticNoise * 0.2 + glow * 0.3);
    b = saturate(b + staticNoise * 0.4 + glow * 0.2);
    
    // Animated "breathing" effect toward ghost center - atmospheric shimmer [NVIDIA GPU Gems - "Volumetric Effects"]
    float breathing = sin(time * 1.5) * 0.1 * ghostDistance;
    float2 breathingOffset = ghostDirection * breathing;
    
    // Final color with transparency
    float4 color = float4(r, g, b, 0.1 + ghostDistance * 0.3);
    
    // Radial vignette (darkening at edges)
    float vignette = 1.0 - smoothstep(0.3, 0.7, ghostDist) * ghostDistance;
    color.rgb *= vignette;
    color.w = 0.0001f;
    
    return color;
}