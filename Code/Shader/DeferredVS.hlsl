#include "Common.hlsli"

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;

    float4 localPos = float4(input.Position, 1.0f);

    // 行列は「position * World * View * Projection」
    float4 worldPos = mul(localPos, World);
    float4 viewPos = mul(worldPos, View);
    output.Position = mul(viewPos, Projection);

    float4 localN = float4(input.Normal, 0.0f);
    output.Normal = mul(localN, World);

    output.WorldPosition = worldPos;
    output.TexCoord = input.TexCoord;
    output.Color = input.Color;
    return output;
}