#include "Common.hlsli"

PS_OUTPUT_GEOMETRY main(PS_INPUT input)
{
    PS_OUTPUT_GEOMETRY output;
    
    output.Color = TextureBaseColor.Sample(Sampler, input.TexCoord);
    output.Normal = input.Normal;
    output.Normal.a = 1.0f;
    
    return output;
}