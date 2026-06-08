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

    float roughness = 0.5f;
    float metallic = 0.0f;

    float3 diffuse = 0.0f;
    {
        float4 light = LightColor * saturate(dot(lightDirection, norm));
        float4 kD = baseColor;
        kD.a = 0.0f;
        diffuse = kD.xyz * light.xyz / PI;
        
        // Smoothly fade diffuse lighting at silhouette edges to match G-Buffer coverage
        diffuse = diffuse * smoothstep(0.1f, 0.9f, normalLength);
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

        float3 F0 = float3(0.5f, 0.5f, 0.5f);
        float3 F;
        float power = (-5.55473f * VoH - 6.98316f) * VoH;
        F = F0 + (1.0f - F0) * exp2(power);
        
        specular = F * g * d / (4.0f * NoV * NoL + 0.001f);
        
        // Smoothly fade specular intensity at silhouette edges to prevent halo artifacts
        specular = specular * smoothstep(0.1f, 0.9f, normalLength);
    }

    float3 ambient = 0.0f;
    {

    }
    
    output.Color.xyz = diffuse + specular + ambient;
    output.Color.a = 1.0f;

    return output;
}
