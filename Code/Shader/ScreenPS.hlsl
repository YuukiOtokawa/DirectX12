#include "Common.hlsli"

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
    
    output.Color = TextureBaseColor.Sample(Sampler, input.TexCoord);
    
    return output;
}