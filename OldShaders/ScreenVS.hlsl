#include "Common.hlsli"

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;

    // すでに -1..1 の頂点をVBに入れているのでそのまま使う
    output.Position = float4(input.Position.xy, 0.0f, 1.0f);

    output.WorldPosition = output.Position;
    output.Normal = float4(0.0f, 0.0f, 1.0f, 0.0f);
    output.TexCoord = input.TexCoord;
    output.Color = input.Color;
    return output;
}