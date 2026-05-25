#include "Common.hlsli"

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
    
    float4 baseColor = TextureBaseColor.Sample(Sampler, input.TexCoord);
    float4 normal = TextureNormal.Sample(Sampler, input.TexCoord);
    
    output.Color = baseColor * saturate(dot(LightDirection.xyz, normal.xyz));
    
    return output;

}