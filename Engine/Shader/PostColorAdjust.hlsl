// Unity風 per-pass プロパティを持つポストプロセスのデモ／検証用シェーダ。
// Common.hlsli の共有マテリアル定義を無効化し、独自の cbuffer を register(b3) に定義する。
#define CUSTOM_MATERIAL_CBUFFER
#include "Common.hlsli"

// HLSL の明示的 cbuffer メンバは初期化子(= 値)を書けないため、
// デフォルト値は [Default(...)] アノテーションで指定する。
cbuffer PostProcessParams : register(b3)
{
    float3 _Tint;        // [Vector] [Default(1,1,1)] "Tint (RGB)"
    float  _Brightness;  // [Range(-1,1)] [Default(0)] "Brightness"
    float  _Contrast;    // [Range(0,2)] [Default(1)] "Contrast"
    float  _Saturation;  // [Range(0,2)] [Default(1)] "Saturation"
};

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

    float3 c = TextureBaseColor.Sample(Sampler, input.TexCoord).rgb;

    // Tint
    c *= _Tint;
    // Brightness
    c += _Brightness;
    // Contrast (0.5 を中心にスケール)
    c = (c - 0.5f) * _Contrast + 0.5f;
    // Saturation
    float gray = dot(c, float3(0.299f, 0.587f, 0.114f));
    c = lerp(float3(gray, gray, gray), c, _Saturation);

    output.Color = float4(saturate(c), 1.0f);
    return output;
}
