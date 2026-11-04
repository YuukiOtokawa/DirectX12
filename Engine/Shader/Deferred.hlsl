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
    float4 matParams = TextureMaterial.Sample(Sampler, input.TexCoord);
    float4 emission = TextureEmission.Sample(Sampler, input.TexCoord);

    // 深度は補間すると物体境界で無意味な値になるので Load で読む
    // （G-Bufferと深度は同じ 1920x1080 で、このパスのビューポートも同じ）
    float depth = TextureDepth.Load(int3(input.Position.xy, 0));

    // 何も描かれていない画素（深度がクリア値1.0のまま）はライティングせずベースカラー（クリア色）を出す
    if (depth >= 1.0f)
    {
        output.Color = baseColor;
        output.Color.a = 1.0f;
        return output;
    }

    // 法線は八面体エンコードの2成分。デコード結果は単位ベクトル
    // 八面体の折り返しをまたいで補間しないよう、深度と同じく Load で読む
    float3 norm = DecodeNormalOct(TextureNormal.Load(int3(input.Position.xy, 0)).xy);

    // 深度からワールド座標を復元する
    float2 ndcXY = input.TexCoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f); // UV(0..1, Vは下向き) -> NDC(-1..+1, Yは上向き)
    float4 worldPosition = mul(float4(ndcXY, depth, 1.0f), InverseViewProjection);
    float3 position = worldPosition.xyz / worldPosition.w;

    float3 eye = CameraPosition.xyz - position.xyz;
    eye = normalize(eye);

    float3 lightDirection = normalize(LightDirection.xyz);
    float3 halfv = normalize(lightDirection + eye);

    // Use saturate to clamp dot products and prevent invalid light reflections or NaN
    float NoH = saturate(dot(norm, halfv));
    float NoV = saturate(dot(norm, eye));
    float NoL = saturate(dot(norm, lightDirection));
    float VoH = saturate(dot(eye, halfv));

    float ambientocclusion = matParams.r; // r: Ambient Occlusion
    float roughness = max(0.01f, matParams.g); // g: Roughness
    float metallic = matParams.b;              // b: Metallic

    // Shadow mapping (CSM): 内側（高解像度）のカスケードから順に試し、範囲内の最初のものを使う
    float shadowFactor = 1.0f;
    {
        // LightViewは全カスケード共通なので先に1回だけ掛ける
        float4 lightViewPos = mul(float4(position.xyz, 1.0f), LightView);

        for (int c = 0; c < 3; c++)
        {
            float4 shadowPos = mul(lightViewPos, CascadeProjection[c]);
            float3 shadowNDC = shadowPos.xyz / shadowPos.w; // Directional(ortho)はw=1

            // NDC(-1..+1, Yは上向き) -> UV(0..1, Vは下向き)
            float2 shadowUV = shadowNDC.xy * float2(0.5f, -0.5f) + 0.5f;

            // このカスケードの範囲外なら次の（より広い）カスケードへ。
            // 枠の境界でPCFのタップが隣のカスケードを拾わないよう、少し内側までに制限
            if (any(shadowUV < 0.002f) || any(shadowUV > 0.998f) || shadowNDC.z > 1.0f)
                continue;

            // 枠内UV(0..1) -> アトラス(2x2グリッド)上のUVへ
            float2 atlasUV = shadowUV * 0.5f + float2((c % 2) * 0.5f, (c / 2) * 0.5f);

            // PCF 3x3: 周囲9テクセルで「比較してから平均」して影の輪郭を柔らかくする
            // （深度を平均してから比較すると、物体境界で無意味な中間深度になるのでNG）
            const float atlasTexel = 1.0f / 2048.0f; // アトラス上の1テクセル（=枠1024pxの1テクセル）

            float sum = 0.0f;
            [unroll]
            for (int y = -1; y <= 1; y++)
            {
                [unroll]
                for (int x = -1; x <= 1; x++)
                {
                    float mapDepth = TextureShadow.Sample(SamplerClamp, atlasUV + float2(x, y) * atlasTexel).r;
                    // バイアスで自己遮蔽の縞（シャドウアクネ）を防ぐ
                    sum += (shadowNDC.z - 0.005f) > mapDepth ? 0.0f : 1.0f;
                }
            }
            shadowFactor = sum / 9.0f;
            break;
        }
    }

    float3 diffuse = 0.0f;
    {
        // 直接光のみ影で遮る（IBLは環境光なのでそのまま）
        float4 light = LightColor * saturate(dot(lightDirection, norm)) * shadowFactor;

        //IBL（正規化した法線で引く。以前は正規化前の法線で acos(normal.y) を計算しており、拡大した物体でNaNになっていた）
        float2 iblTexcoord;
        iblTexcoord.x = -atan2(norm.x, norm.z) / (PI * 2);
        iblTexcoord.y = acos(norm.y) / PI;
        light += TextureEnviroment.SampleLevel(Sampler, iblTexcoord, 9) * (1 - metallic) * 10;

        diffuse = light * baseColor.xyz / PI;
    }

    float3 specular = 0.0f;
    { // cook-torrance microfacet model
        float a = roughness * roughness;
        float a2 = a * a;
        float d = (NoH * NoH) * (a2 - 1.0f) + 1.0f;
        d = a2 / (PI * d * d);

        float g;
        float k = ((roughness + 1.0f) * (roughness + 1.0f)) / 8.0f;
        float g1V = NoV / (NoV * (1.0f - k) + k);
        float g1L = NoL / (NoL * (1.0f - k) + k);
        g = g1V * g1L;

        float3 F0 = lerp(0.04f, baseColor.xyz, metallic);
        float3 F;
        float power = (-5.55473f * VoH - 6.98316f) * VoH;
        F = F0 + (1.0f - F0) * pow(2.0f, power);
        
        specular = F * g * d / (4.0f * NoV * NoL + 0.001f);
    }
    
    // スペキュラも直接光由来なので影で遮る。emissionは自己発光なので影の影響を受けない
    output.Color.xyz = diffuse + specular * shadowFactor + emission.xyz;
    output.Color.a = 1.0f;

    return output;
}
