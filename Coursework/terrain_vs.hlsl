/** Terrain: Vertex Shader Code **/

/****************************************************************************************************************************/

// Struct to define the input to the vertex shader
struct InputType
{
    float3 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

/****************************************************************************************************************************/

// Struct to define the output to the vertex shader
struct OutputType
{
    float3 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

/****************************************************************************************************************************/

// Main vertex shader function
OutputType main(InputType input)
{
    OutputType output;
    
    output.position = input.position;
    output.tex = input.tex;
    output.normal = normalize(input.normal);
    
    return output;
}