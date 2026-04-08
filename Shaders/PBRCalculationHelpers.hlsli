static const float PI = 3.14159265;

// Fresnel (Schlick)
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// Normal Distribution (GGX)
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;

    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom + 1e-5);
}

// Geometry (Smith GGX)
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}




float3 ShadePBR(
    float3 N,
    float3 V,
    float3 worldPos,
    float2 uv
)
{
    // --- Sample material ---
    float3 baseColor = baseColorFactor.rgb;

    if (baseColorTex)
    {
        float3 tex = baseColorTex.Sample(samp, uv).rgb;
        tex = pow(tex, 2.2); // sRGB -> linear
        baseColor *= tex;
    }

    float metallic = metallicFactor;
    float roughness = roughnessFactor;

    if (metallicRoughTex)
    {
        float2 mr = metallicRoughTex.Sample(samp, uv).bg;
        metallic *= mr.y;
        roughness *= mr.x;
    }

    roughness = clamp(roughness, 0.04, 1.0);

    // --- Lighting setup ---
    float3 L = normalize(-lightDirection);
    float3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    // --- Fresnel base reflectance ---
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), baseColor, metallic);

    float3 F = FresnelSchlick(VdotH, F0);
    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);

    float3 numerator = D * G * F;
    float denom = 4.0 * NdotV * NdotL + 1e-5;

    float3 specular = numerator / denom;

    // --- Diffuse ---
    float3 kS = F;
    float3 kD = (1.0 - kS) * (1.0 - metallic);

    float3 diffuse = kD * baseColor / PI;

    // --- Shadow (hook) ---
    float shadow = 1.0;

    // Example: call your shadow ray here
    // shadow = TraceShadowRay(worldPos, L);

    // --- Final ---
    float3 color = (diffuse + specular) * lightColor * NdotL * shadow;

    return color;
}