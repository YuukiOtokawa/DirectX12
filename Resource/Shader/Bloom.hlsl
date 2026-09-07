#define CUSTOM_MATERIAL_CBUFFER
#include "Common.hlsli"

//==================================================
// Bloom
//   ミップピラミッド + 各段で分離ガウス（URP方式）
//   1ファイル5エントリポイント。PSOは RenderManager 側で
//   CreatePipeline(..., "vtx", "pixXxx") で個別に作る。
//
//   パス    | 入力 t0        | 入力 t7      | 出力
//   --------+----------------+--------------+--------------
//   Prefilter | LightedColor | -            | MipDown[0]
//   BlurH     | MipDown[i-1] | -            | MipUp[i]     ※ここで半分に縮む
//   BlurV     | MipUp[i]     | -            | MipDown[i]
//   Upsample  | MipDown[i]   | 低ミップ側   | MipUp[i]
//   Composite | LightedColor | MipUp[0]     | PostProcessBuffer1
//==================================================

cbuffer BloomConstantBuffer : register(b3)
{
    float  Threshold;      // 輝度の抽出閾値
    float  Knee;           // ソフトニーの幅
    float  Intensity;      // 合成時の強度
    float  Scatter;        // アップサンプル時の広がり（低ミップ寄りにする度合い）
    float2 InputTexel;     // t0 側の 1/幅, 1/高さ  ※必ず「入力」のテクセル
    float2 LowMipTexel;    // t7 側の 1/幅, 1/高さ
};

//--------------------------------------------------
// ヘルパー
//--------------------------------------------------

// s0(Sampler) は ANISOTROPIC + WRAP なので画面端が反対側から滲む。
// フルスクリーン処理では必ず s1(SamplerClamp / LINEAR + CLAMP) を使う。
float3 SampleInput(float2 uv)
{
    return TextureBaseColor.SampleLevel(SamplerClamp, uv, 0.0f).rgb;
}

float3 SampleLow(float2 uv)
{
    return TextureSceneColor.SampleLevel(SamplerClamp, uv, 0.0f).rgb;
}

float Max3f(float a, float b, float c)
{
    return max(a, max(b, c));
}

// 相対輝度(dot)だと純青のネオンのような彩度の高い色を拾い漏らすので
// URP と同じく RGB の最大値を使う。
float Brightness(float3 c)
{
    return Max3f(c.r, c.g, c.b);
}

// Karis average: 明るいタップほど重みを下げ、
// 1ピクセルだけ極端に明るい点（ファイアフライ）が
// ミップを下るたびに点滅するのを抑える。
float KarisWeight(float3 c)
{
    return 1.0f / (1.0f + Brightness(c));
}

//--------------------------------------------------
// 頂点シェーダ（5パス共通）
//--------------------------------------------------

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

//--------------------------------------------------
// Prefilter : 輝度抽出 + 4タップのダウンサンプル
//--------------------------------------------------

PS_OUTPUT pixPrefilter(PS_INPUT input)
{
    PS_OUTPUT output;

    float2 uv = input.TexCoord;
    float2 t  = InputTexel;

    // 各タップがバイリニアで 2x2 を拾うので実質 4x4 相当
    float3 c0 = SampleInput(uv + float2(-t.x, -t.y));
    float3 c1 = SampleInput(uv + float2( t.x, -t.y));
    float3 c2 = SampleInput(uv + float2(-t.x,  t.y));
    float3 c3 = SampleInput(uv + float2( t.x,  t.y));

    float w0 = KarisWeight(c0);
    float w1 = KarisWeight(c1);
    float w2 = KarisWeight(c2);
    float w3 = KarisWeight(c3);

    float3 color = (c0 * w0 + c1 * w1 + c2 * w2 + c3 * w3) / max(w0 + w1 + w2 + w3, 1e-4f);

    // ソフトニー閾値（Karis）。ハードな閾値だと境界がくっきり出てしまう。
    // RT が R16G16B16A16_FLOAT なので 1.0 超えの HDR 値がここまで保持されている。
    float brightness = Brightness(color);
    float softness = clamp(brightness - Threshold + Knee, 0.0f, 2.0f * Knee);
    softness = (softness * softness) / (4.0f * Knee + 1e-4f);
    float multiplier = max(brightness - Threshold, softness) / max(brightness, 1e-4f);

    color *= multiplier;

    output.Color = float4(color, 1.0f);
    return output;
}

//--------------------------------------------------
// BlurH : 横9タップ。出力が入力の半分なのでダウンサンプルも兼ねる
//--------------------------------------------------

PS_OUTPUT pixBlurH(PS_INPUT input)
{
    PS_OUTPUT output;

    float2 uv = input.TexCoord;
    float  dx = InputTexel.x;

    float3 color;
    color  = SampleInput(uv + float2(-dx * 4.0f, 0.0f)) * 0.01621622f;
    color += SampleInput(uv + float2(-dx * 3.0f, 0.0f)) * 0.05405405f;
    color += SampleInput(uv + float2(-dx * 2.0f, 0.0f)) * 0.12162162f;
    color += SampleInput(uv + float2(-dx * 1.0f, 0.0f)) * 0.19459459f;
    color += SampleInput(uv                           ) * 0.22702703f;
    color += SampleInput(uv + float2( dx * 1.0f, 0.0f)) * 0.19459459f;
    color += SampleInput(uv + float2( dx * 2.0f, 0.0f)) * 0.12162162f;
    color += SampleInput(uv + float2( dx * 3.0f, 0.0f)) * 0.05405405f;
    color += SampleInput(uv + float2( dx * 4.0f, 0.0f)) * 0.01621622f;

    output.Color = float4(color, 1.0f);
    return output;
}

//--------------------------------------------------
// BlurV : 縦5タップ。サイズは変えない
//--------------------------------------------------

PS_OUTPUT pixBlurV(PS_INPUT input)
{
    PS_OUTPUT output;

    float2 uv = input.TexCoord;
    float  dy = InputTexel.y;

    float3 color;
    color  = SampleInput(uv + float2(0.0f, -dy * 2.0f)) * 0.07027027f;
    color += SampleInput(uv + float2(0.0f, -dy * 1.0f)) * 0.31621622f;
    color += SampleInput(uv                           ) * 0.22702703f;
    color += SampleInput(uv + float2(0.0f,  dy * 1.0f)) * 0.31621622f;
    color += SampleInput(uv + float2(0.0f,  dy * 2.0f)) * 0.07027027f;

    output.Color = float4(color, 1.0f);
    return output;
}

//--------------------------------------------------
// Upsample : 低ミップ(t7)を9タップtentで拡大し、同段(t0)と混ぜる
//--------------------------------------------------

float3 UpsampleTent(float2 uv, float2 texel)
{
    float3 s;
    s  = SampleLow(uv + float2(-texel.x,  texel.y)) * 1.0f;
    s += SampleLow(uv + float2(     0.0f,  texel.y)) * 2.0f;
    s += SampleLow(uv + float2( texel.x,  texel.y)) * 1.0f;

    s += SampleLow(uv + float2(-texel.x,     0.0f)) * 2.0f;
    s += SampleLow(uv                              ) * 4.0f;
    s += SampleLow(uv + float2( texel.x,     0.0f)) * 2.0f;

    s += SampleLow(uv + float2(-texel.x, -texel.y)) * 1.0f;
    s += SampleLow(uv + float2(     0.0f, -texel.y)) * 2.0f;
    s += SampleLow(uv + float2( texel.x, -texel.y)) * 1.0f;

    return s * (1.0f / 16.0f);
}

PS_OUTPUT pixUpsample(PS_INPUT input)
{
    PS_OUTPUT output;

    float2 uv = input.TexCoord;

    float3 high = SampleInput(uv);
    float3 low  = UpsampleTent(uv, LowMipTexel);

    // 加算ではなく lerp にしておくとエネルギーが発散せず、
    // Scatter で「広がり」を直感的に調整できる。
    output.Color = float4(lerp(high, low, Scatter), 1.0f);
    return output;
}

//--------------------------------------------------
// Composite : 元のシーン(t0) にブルーム(t7) を加算
//--------------------------------------------------

PS_OUTPUT pixComposite(PS_INPUT input)
{
    PS_OUTPUT output;

    float2 uv = input.TexCoord;

    float3 scene = SampleInput(uv);
    float3 bloom = SampleLow(uv);

    output.Color = float4(scene + bloom * Intensity, 1.0f);
    return output;
}
