#include "Common.hlsli"

PS_INPUT vtx(VS_INPUT input)
{
    PS_INPUT output;

    // シャドウマップはライト視点で描く（カメラのView/Projectionではない）
    float4x4 wvp = mul(World, LightView);
    wvp = mul(wvp, LightProjection);
    output.Position = mul(float4(input.Position, 1.0f), wvp);

    // ライト空間のクリップ座標をそのまま渡し、PS側でwで割ってNDC深度にする
    output.WorldPosition = output.Position;

    output.Normal = mul(float4(input.Normal, 0.0f), World);
    output.TexCoord = input.TexCoord;
    output.Color = input.Color;

    return output;
}

PS_OUTPUT pix(PS_INPUT input)
{
    PS_OUTPUT output;

    // NDC深度(0..1)。Directional(ortho)ならw=1なのでzがそのまま線形深度
    float depth = input.WorldPosition.z / input.WorldPosition.w;
    output.Color = float4(depth, 0.0, 0.0, 1.0);

    return output;
}