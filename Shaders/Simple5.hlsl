#include "SimpleVSPSInterface.hlsli"
#include "CameraBuffer.hlsli"

//needs a sampler and srv
Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);


VSOutput_5 VSMain(VSInput_5 input)
{
    VSOutput_5 output;
    
    //if transposed on CPU then hlsl reads this as row-major
    //in row-major, v' = v * M
    output.position = mul(float4(input.position, 1.0f), g_mvpT);
    output.worldPosition = mul(float4(input.position, 1.0f), g_modelMatrixT);
    output.normal = normalize(mul(input.normal, (float3x3)g_normalMatrix));
   
    output.texcoord0 = input.texcoord0;
    output.texcoord1 = input.texcoord1;
    output.texcoord2 = input.texcoord2;
    
    return output;
}

// GGX / Trowbridge-Reitz NDF
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness; // roughness squared
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    denom = PI * denom * denom;

    return a2 / denom;
}

float GeometrySchlickGGX(float NdotX, float roughness)
{
    float a = roughness * roughness;
    float k = a / 2.0;

    return NdotX / (NdotX * (1.0 - k) + k);
}


float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    float ggxV = GeometrySchlickGGX(NdotV, roughness);
    float ggxL = GeometrySchlickGGX(NdotL, roughness);

    return ggxV * ggxL;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}




float4 PSMain(PSInput_5 input) : SV_TARGET
{
    ///@todo Pass it from CPU
    float3 LightDirection = normalize(float3(-0.4f, -1.0f, -0.3f));
    float3 LightColor = float3(1.0f, 0.98f, 0.95f);
    float  LightIntensity = 8.0f;
    
    // Default material values
    float metallic = 0.0; // organic
    float roughness = 0.8; // fur / skin
    
    // Dielectric reflectance
    float3 F0 = float3(0.04, 0.04, 0.04);
    
    float3 N = normalize(input.normal);
    float3 V = normalize(g_cameraPosition - input.worldPosition).xyz;
    float3 L = normalize(-LightDirection);
    float3 H = normalize(V + L);

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
    
    float3 sampledColor = gTexture.Sample(gSampler, input.texcoord0).xyz;
    float3 albedo = pow(abs(sampledColor), 2.2);
    
    float3 specular =
        (NDF * G * F) /
        (4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001);
    
    float3 kS = F;
    float3 kD = 1.0 - kS;

    float NdotL = max(dot(N, L), 0.0);

    float3 color =
        (kD * albedo / PI + specular) *
        LightColor * LightIntensity * NdotL;
    
    
    color = pow(N, 1.0 / 2.2);
    
    return float4(color, 1.0f);

}