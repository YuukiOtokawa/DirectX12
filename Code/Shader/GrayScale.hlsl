#include "Common.hlsli"

PS_INPUT vtx(VS_INPUT input)
{
    PS_INPUT output;

    output.Position = float4(input.Position.xy, 0.0f, 1.0f);

    output.WorldPosition = output.Position;
    output.Normal = float4(0.0f, 0.0f, 1.0f, 0.0f);
    output.TexCoord = input.TexCoord;
    output.Color = input.Color;
    return output;
}

PS_OUTPUT pix(PS_INPUT input)
{
    PS_OUTPUT output;
    
    float4 baseColor = TextureBaseColor.Sample(Sampler, input.TexCoord);
    
    float gray = dot(baseColor.rgb, float3(0.299f, 0.587f, 0.114f));

    output.Color.xyz = float3(gray, gray, gray);
    output.Color.a = 1.0f;

    return output;
}
