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
    float4 normal = TextureNormal.Sample(Sampler, input.TexCoord);
    float4 position = TexturePosition.Sample(Sampler, input.TexCoord);
    float4 matParams = TextureMaterial.Sample(Sampler, input.TexCoord);
    float4 emission = TextureEmission.Sample(Sampler, input.TexCoord);

    // Calculate normal length to identify background and silhouette edges
    float normalLength = length(normal.xyz);
    
    // If it is completely background, output baseColor directly without lighting
    if (normalLength < 0.1f)
    {
        output.Color = baseColor;
        output.Color.a = 1.0f;
        return output;
    }

    // Safely normalize the normal vector to prevent NaN on silhouette edges
    float3 norm = normal.xyz / normalLength;

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

        //IBL
        float2 iblTexcoord;
        iblTexcoord.x = -atan2(normal.x, normal.z) / (PI * 2);
        iblTexcoord.y = acos(normal.y) / PI;
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
        
        // Smoothly fade specular intensity at silhouette edges to prevent halo artifacts
        specular = specular * smoothstep(0.1f, 0.9f, normalLength);
    }
    
    // スペキュラも直接光由来なので影で遮る。emissionは自己発光なので影の影響を受けない
    output.Color.xyz = diffuse + specular * shadowFactor + emission.xyz;
    output.Color.a = 1.0f;

    return output;
}
