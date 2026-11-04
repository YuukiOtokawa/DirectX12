


cbuffer EnvConstantBuffer : register(b0)
{
    float4 LightDirection;
    float4 LightColor;
    float Exposure;
    float3 Padding;
    float4x4 LightView;
    float4x4 LightProjection;             // シャドウパス用（今描いているカスケードの射影）
    float4x4 CascadeProjection[3];        // Deferredサンプリング用（全カスケード、近→遠）
};


cbuffer CameraConstantBuffer : register(b1)
{
    float4x4 View;
    float4x4 Projection;
    float4 CameraPosition;
    float4x4 InverseViewProjection;       // Deferredで深度からワールド座標を復元する用

};


cbuffer ObjectConstantBuffer : register(b2)
{
    float4x4 World;
};


// シェーダ側で独自のマテリアル cbuffer (register(b3)) を定義したい場合は、
// Common.hlsli を include する前に CUSTOM_MATERIAL_CBUFFER を #define する。
// その場合、下記の共有マテリアル定義は無効化される（Unity風の per-shader プロパティ運用）。
#ifndef CUSTOM_MATERIAL_CBUFFER
cbuffer SubsetConstantBuffer : register(b3)
{
    struct MATERIAL
    {
        float4 BaseColor;
        float4 EmissionColor; // [HDR] [Default(0,0,0,1)] "Emission Color"
        float Metallic;
        float Specular;
        float Roughness;
        float NormalWeight;
    } Material;
};
#endif






struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD;
    float4 Color : COLOR;
};


struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 WorldPosition : POSITION;
    float4 Normal : NORMAL;
    float2 TexCoord : TEXCOORD;
    float4 Color : COLOR;
};

// G-Buffer。フォーマットは RenderTargetManager::GBUFFER_FORMATS と合わせる
struct PS_OUTPUT_GEOMETRY
{
    float4 Color : SV_TARGET0;    // R8G8B8A8_UNORM  rgb: ベースカラー（0..1に丸められる）
    float2 Normal : SV_TARGET1;   // R16G16_SNORM    EncodeNormalOct() した正規化ワールド法線
    float4 Material : SV_TARGET2; // R8G8B8A8_UNORM  r: AO, g: Roughness, b: Metallic
    float4 Emission : SV_TARGET3; // R11G11B10_FLOAT rgb: HDRの発光（aと負の値は保存されない）
    // ワールド座標は書かない。Deferredで深度バッファ(TextureDepth)から復元する
};

struct PS_OUTPUT
{
    float4 Color : SV_TARGET0;
};


Texture2D<float4> TextureBaseColor : register(t0);
Texture2D<float4> TextureNormal : register(t1);
Texture2D<float> TextureDepth : register(t2); // メイン深度バッファ（R32_FLOAT）
Texture2D<float4> TextureMaterial : register(t3);
Texture2D<float4> TextureEmission : register(t4);
Texture2D<float4> TextureEnviroment : register(t5);
Texture2D<float4> TextureShadow : register(t6);
Texture2D<float4> TextureSceneColor : register(t7);

SamplerState Sampler : register(s0);
SamplerState SamplerClamp : register(s1); // シャドウマップ用（WRAPだと範囲外で影が繰り返す）

static float PI = 3.14159265359f;


// 単位ベクトルを八面体エンコードで2成分(-1..1)にする。法線G-Buffer（R16G16_SNORM）用
// （ランタイムはfxcでコンパイルするので select() ではなく成分ごとの三項演算子を使う）
float2 OctWrap(float2 v)
{
    return (1.0f - abs(v.yx)) * (v.xy >= 0.0f ? 1.0f : -1.0f);
}

float2 EncodeNormalOct(float3 n)
{
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    return n.z >= 0.0f ? n.xy : OctWrap(n.xy);
}

float3 DecodeNormalOct(float2 f)
{
    float3 n = float3(f.x, f.y, 1.0f - abs(f.x) - abs(f.y));
    float t = saturate(-n.z);
    n.xy += (n.xy >= 0.0f ? -t : t);
    return normalize(n);
}