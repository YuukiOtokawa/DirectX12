#include "Common.hlsli"

// 画面空間微分から接線基底を作り、接線空間法線(tn)をワールド法線へ変換（Approach B）
float3 PerturbNormal(float3 N, float3 worldPos, float2 uv, float3 tn)
{
    float3 dp1  = ddx(worldPos);
    float3 dp2  = ddy(worldPos);
    float2 duv1 = ddx(uv);
    float2 duv2 = ddy(uv);

    float3 dp2perp = cross(dp2, N);
    float3 dp1perp = cross(N, dp1);
    float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    float3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    float invmax = rsqrt(max(dot(T, T), dot(B, B)));
    float3x3 TBN = float3x3(T * invmax, B * invmax, N);
    return normalize(mul(tn, TBN));
}

// register space1 の法線マップ。未割当時は白(1,1,1)ダミーが入る。
Texture2D _NormalMap : register(t0, space1);  // [Texture] "Normal Map"
Texture2D _ArmMap : register(t1, space1); // [Texture] "ARM Map"

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

    float3 N = normalize(input.Normal.xyz);
    float origLen = length(input.Normal.xyz); // 元の長さ（Deferredのシルエット減衰用）を保持

    // 法線マップ（未割当=白ダミーのときはフラット法線として素通し）
    // ダミーは非圧縮1x1で必ず厳密に(1,1,1)になるため、圧縮アーティファクトによる
    // 誤判定を避けるためイコール判定にしている（>0.99fだとBC1等の量子化誤差で
    // 白に近い実データを未割当と誤認する場合がある）
    float3 raw = _NormalMap.Sample(Sampler, input.TexCoord).xyz;
    float3 worldN = N;
    if (!(raw.r == 1.0f && raw.g == 1.0f && raw.b == 1.0f))
    {
        float3 tn = raw * 2.0f - 1.0f;
        tn.xy *= Material.NormalWeight;   // NormalWeight を法線強度として流用
        tn = normalize(tn);
        worldN = PerturbNormal(N, input.WorldPosition.xyz, input.TexCoord, tn);
    }

    output.Color    = TextureBaseColor.Sample(Sampler, input.TexCoord) * Material.BaseColor;
    output.Normal   = float4(worldN * origLen, 1.0f); // 未割当時は元の input.Normal と一致
    output.Position = input.WorldPosition;

    // G-Bufferにマテリアル属性を書き込む
    output.Material = _ArmMap.Sample(Sampler, input.TexCoord);
    output.Material *= float4(Material.Metallic, Material.Specular, Material.Roughness, Material.NormalWeight);
    output.Emission = Material.EmissionColor;

    return output;
}
