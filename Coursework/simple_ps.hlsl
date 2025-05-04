Texture2D sceneTexture : register(t0);
SamplerState sampleState : register(s0);

struct InputType
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

float4 main(InputType input) : SV_TARGET
{
    //float4 textureColor = sceneTexture.Sample(sampleState, input.texCoord);
    //return textureColor;
    
    float2 uv = input.texCoord;
    float2 center = float2(0.5, 0.5);

    // direction & distance from center
    float2 d = uv - center;
    float dist = length(d);

    // animate ripple over time—use SV_Time or a shader constant, here just a static ripple:
    float ripple = sin(dist * 30.0) * 0.5 + 0.5;
    // sin(...) gives [ -1 .. 1 ]; map to [ 0 .. 1 ]

    // base color ramps from dark in the middle to bright at peaks of the sine
    float base = lerp(0.1, 1.0, ripple);

    // add a soft vignette (darken edges)
    float vignette = 1.0 - smoothstep(0.5, 0.8, dist);

    // final color: bluish tint modulated by ripple and vignette
    float3 color = float3(0.2, 0.4, 0.8) * base * vignette;

    return float4(color, 1.0);
}