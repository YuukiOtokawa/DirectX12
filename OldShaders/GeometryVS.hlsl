#include "Common.hlsli"

PS_INPUT main(VS_INPUT input)
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