// Unity風 per-shader マテリアルプロパティのデモ／検証用シェーダ。
// Common.hlsli の共有マテリアル定義を無効化し、独自の cbuffer を register(b3) に定義する。
#define CUSTOM_MATERIAL_CBUFFER
#include "Common.hlsli"

// 各フィールドの行末コメントが Inspector のUIに反映される:
//   "..."            表示名
//   [Range(min,max)] スライダー範囲（float）
//   [Color]/[Vector] float3/float4 のUI切り替え
//   [Header(...)]    区切りヘッダ
//   [Default(...)]   初期値（HLSLのcbufferメンバは初期化子を書けないため）
cbuffer MaterialProperties : register(b3)
{
    float4 _BaseColor;   // [Header(Surface)] [Color] [Default(1,1,1,1)] "Base Color"
    float4 _Emission;    // [Color] [Default(0,0,0,1)] "Emission"
    float  _Metallic;    // [Range(0,1)] "Metallic"
    float  _Roughness;   // [Range(0,1)] [Default(0.5)] "Roughness"
    float3 _Tint;        // [Vector] [Default(1,1,1)] "Tint (RGB)"
    float  _Intensity;   // [Range(0,4)] [Default(1)] "Intensity"
};

// register space1 の追加マテリアルテクスチャ（未設定なら白ダミー＝変化なし）
Texture2D _DetailTex : register(t0, space1);  // [Texture] "Detail Texture"

PS_INPUT vtx(VS_INPUT input)
{
    PS_INPUT output;

    float4x4 wvp;
    wvp = mul(World, View);
    wvp = mul(wvp, Projection);

    float4 position = float4(input.Position, 1.0f);
    output.Position = mul(position, wvp);

    float4 normal = float4(input.Normal, 0.0f);
    output.Normal = mul(normal, World);

    output.TexCoord = input.TexCoord;
    output.Color = input.Color;

    return output;
}

PS_OUTPUT pix(PS_INPUT input)
{
    PS_OUTPUT output;

    float4 tex = TextureBaseColor.Sample(Sampler, input.TexCoord);
    float3 rgb = tex.rgb * _BaseColor.rgb * _Tint * _Intensity + _Emission.rgb;

    // space1 の追加テクスチャ（未設定なら白ダミー＝変化なし）
    rgb *= _DetailTex.Sample(Sampler, input.TexCoord).rgb;

    output.Color = float4(rgb, tex.a * _BaseColor.a);
    return output;
}
