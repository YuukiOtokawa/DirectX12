#include "Common.hlsli"

PS_INPUT vtx(VS_INPUT input)
{
    PS_INPUT output;
    
    float4 worldPosition = mul(float4(input.Position, 1.0f), World);
    output.Position = mul(worldPosition, View);
    output.Position = mul(output.Position, Projection);
    
    output.WorldPosition = worldPosition;
    output.Normal = mul(float4(input.Normal, 0.0f), World);
    output.TexCoord = input.TexCoord;
    output.Color = input.Color;
    
    return output;
}

PS_OUTPUT_GEOMETRY pix(PS_INPUT input)
{
    PS_OUTPUT_GEOMETRY output;
    
    output.Color = TextureBaseColor.Sample(Sampler, input.TexCoord) * Material.BaseColor;
    output.Normal = input.Normal;
    output.Normal.a = 1.0f;
    output.Position = input.WorldPosition;
    
    return output;
}
