#include "CameraBuffer.hlsli"

struct HSConstantsTriOutput
{
    float TessLevelOuter[3] : SV_TessFactor;
    float TessLevelInner : SV_InsideTessFactor;
};

struct VSInput_1
{
    float3 position         : POSITION;
};

struct VSInput_2
{
    float3 position         : POSITION;
    float3 normal           : NORMAL;
};


struct VSInput_3
{
    float3 position         : POSITION;
    float3 normal           : NORMAL;
    float2 texcoord0        : TEXCOORD0;
};


struct VSInput_4
{
    float3 position         : POSITION;
    float3 normal           : NORMAL;
    float2 texcoord0        : TEXCOORD0;
    float3 tangent          : TANGENT;
};


struct VSOutput_5
{
    float4 position         : SV_POSITION;
    float4 worldPosition    : WORLDPOSITION;
    float3 normal           : NORMAL;
    float2 texcoord0        : TEXCOORD0;
    float3 tangent          : TANGENT;
    uint   flags            : FLAGS;
};

void ComputePositionOutput(float3 inPosition, out float4 worldPosition, out float4 svPosition)
{
    worldPosition = mul(float4(inPosition, 1.0f), g_modelMatrixT);
    svPosition = mul(worldPosition, sceneConstants.g_viewProjT);
}

void CheckComputePositionOutput(float3 inPosition, out float4 worldPosition, out float4 svPosition)
{
    //if transposed on CPU then hlsl reads this as row-major
    //in row-major, v' = v * M
    if ((sceneConstants.renderFlags & RenderFlagsTessEnabled) == 0)
    {
        ComputePositionOutput(inPosition, worldPosition, svPosition);
    }
    else
    {
        svPosition = float4(inPosition, 1.0f);
    }
}


VSOutput_5 VSMain_1( VSInput_1 input )
{
    VSOutput_5 output;
   
    CheckComputePositionOutput(input.position, output.worldPosition, output.position);

    output.normal     = float3(0, 0, 0);
    output.tangent    = float3(0, 0, 0);
    output.texcoord0  = float2(0, 0);
    
    output.flags = 0;

    return output;
}


VSOutput_5 VSMain_2(VSInput_2 input)
{
    VSOutput_5 output;
 
    CheckComputePositionOutput(input.position, output.worldPosition, output.position);
    
    output.normal     = normalize(mul(input.normal, (float3x3) g_normalMatrix));
    output.tangent    = float3(0, 0, 0);
    output.texcoord0  = float2(0, 0);
    
    output.flags = HasNormal;
    
    return output;
}

VSOutput_5 VSMain_3(VSInput_3 input)
{
    VSOutput_5 output;

    CheckComputePositionOutput(input.position, output.worldPosition, output.position);
 
    output.normal = normalize(mul(input.normal, (float3x3) g_normalMatrix));
    output.texcoord0 = input.texcoord0;
    output.tangent   = float3(0, 0, 0);
    
    output.flags = HasNormal | HasTexCoord;
    
    return output;
}

VSOutput_5 VSMain_4(VSInput_4 input)
{
    VSOutput_5 output;
    
    CheckComputePositionOutput(input.position, output.worldPosition, output.position);
 
    output.normal = normalize(mul(input.normal, (float3x3) g_normalMatrix));
    output.texcoord0 = input.texcoord0;
    output.tangent   = input.tangent;
    
    output.flags = HasNormal | HasTexCoord | HasTangent;
    
    return output;
}

cbuffer RootConstant : register(b0, space2)
{
    float tessLevel;
};

HSConstantsTriOutput HSConstantFunc(InputPatch<VSOutput_5, 3> patch, uint patchId : SV_PrimitiveID)
{
    HSConstantsTriOutput triPatchConstants;
    triPatchConstants.TessLevelInner = tessLevel;
    triPatchConstants.TessLevelOuter[0] = tessLevel;
    triPatchConstants.TessLevelOuter[1] = tessLevel;
    triPatchConstants.TessLevelOuter[2] = tessLevel;
    return triPatchConstants;
}

//----------------------------------- Passthrough HS DS--------------------------------------------------------//
[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("HSConstantFunc")]
[maxtessfactor(20.0f)]
VSOutput_5 HSMain(
    InputPatch<VSOutput_5, 3> patch,
    uint i : SV_OutputControlPointID,
    uint patchId : SV_PrimitiveID)
{
    VSOutput_5 output;

    // Pass through control point position
    output.position      = patch[i].position;
    output.worldPosition = patch[i].worldPosition;
    output.texcoord0     = patch[i].texcoord0;
    output.normal        = patch[i].normal;
    output.tangent       = patch[i].tangent;
    output.flags         = patch[i].flags;
    return output;
}

[domain("tri")]
VSOutput_5 DSMain_Pass(HSConstantsTriOutput tessFactors, float3 bary : SV_DomainLocation, const OutputPatch<VSOutput_5, 3> patch)
{
    VSOutput_5 output = (VSOutput_5) 0;
    output.position = (bary.x * patch[0].position) +
                      (bary.y * patch[1].position) +
                      (bary.z * patch[2].position);
    ComputePositionOutput(output.position.xyz, output.worldPosition, output.position);
   
    output.texcoord0 = patch[0].texcoord0 * bary.x + patch[1].texcoord0 * bary.y + patch[2].texcoord0 * bary.z;
    output.normal    = patch[0].normal    * bary.x + patch[1].normal    * bary.y + patch[2].normal    * bary.z;
    output.tangent   = patch[0].tangent   * bary.x + patch[1].tangent   * bary.y + patch[2].tangent   * bary.z;
    output.worldPosition = patch[0].worldPosition * bary.x + patch[0].worldPosition * bary.y + patch[0].worldPosition * bary.z;
    
    output.flags     = patch[0].flags;
    return output;
}

// -------------------------------------------------------------------------------------------------------------------------------------------------//

[domain("tri")]
VSOutput_5 DSMain_PN(HSConstantsTriOutput input, const OutputPatch<VSOutput_5, 3> patch,
    float3 bary : SV_DomainLocation
)
{
    VSOutput_5 output = (VSOutput_5) 0;
    
    float u = bary.x;
    float v = bary.y;
    float w = bary.z;
    
    // Corner positions
    float3 P0 = patch[0].position.xyz;
    float3 P1 = patch[1].position.xyz;
    float3 P2 = patch[2].position.xyz;
    
    // Corner normals
    float3 N0 = normalize(patch[0].normal);
    float3 N1 = normalize(patch[1].normal);
    float3 N2 = normalize(patch[2].normal);
    
    float3 B300 = P0;
    float3 B030 = P1;
    float3 B003 = P2;
    
     // Edge control points
    float3 B210 = (2.0 * P0 + P1 - dot(P1 - P0, N0) * N0) / 3.0;
    float3 B120 = (2.0 * P1 + P0 - dot(P0 - P1, N1) * N1) / 3.0;
    
    float3 B021 = (2.0 * P1 + P2 - dot(P2 - P1, N1) * N1) / 3.0;
    float3 B012 = (2.0 * P2 + P1 - dot(P1 - P2, N2) * N2) / 3.0;
    
    float3 B102 = (2.0 * P2 + P0 - dot(P0 - P2, N2) * N2) / 3.0;
    float3 B201 = (2.0 * P0 + P2 - dot(P2 - P0, N0) * N0) / 3.0;
    
    // Interior control point
    float3 E =
        (B210 + B120 +
         B021 + B012 +
         B102 + B201) / 6.0;
    
    float3 V = (P0 + P1 + P2) / 3.0;

    float3 B111 = E + (E - V) * 0.5;
    
      // --- Cubic Bernstein basis ---
    float u2 = u * u;
    float v2 = v * v;
    float w2 = w * w;

    float u3 = u2 * u;
    float v3 = v2 * v;
    float w3 = w2 * w;

    float3 position =
          B300 * w3
        + B030 * u3
        + B003 * v3
        + B210 * 3.0 * w2 * u
        + B120 * 3.0 * w * u2
        + B021 * 3.0 * u2 * v
        + B012 * 3.0 * u * v2
        + B102 * 3.0 * v2 * w
        + B201 * 3.0 * v * w2
        + B111 * 6.0 * u * v * w;

    // --- Smooth normal interpolation ---
    float3 normal =
        normalize(
            N0 * w +
            N1 * u +
            N2 * v
        );

    output.position = float4(position, 1.0);
    output.normal = normal;
    output.texcoord0 = patch[0].texcoord0 * bary.x + patch[1].texcoord0 * bary.y + patch[2].texcoord0 * bary.z;
    
    ComputePositionOutput(output.position.xyz, output.worldPosition, output.position);
    
    output.flags = patch[0].flags;

    return output;
}

// ------------------------------------------------------------------------------------------------------------------------------------------//

SamplerState gSampler : register(s0);

Texture2D pbrBaseColorTexture         : register(t0);
Texture2D pbrMetallicRoughnessTexture : register(t1);
Texture2D normalTexture               : register(t2);
Texture2D occlusionTexture            : register(t3);
Texture2D emissiveTexture             : register(t4);


//returns density of microfacets aligned with half vector
//some of these might not be visible due to surface geometry
//So use GeometrySchlickGGX
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;

    float NdotH = saturate(dot(N, H));
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (3.14159265 * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    return NdotV / (NdotV * (1.0 - k) + k);
}

//returns the density of microfacets visible - light reaches and seen by camera
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));

    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
    

float4 PSMain(VSOutput_5 input) : SV_TARGET
{
    float3 lightDir   = float3(1, -1, 1);
    float4 lightColor = float4(1, 0.8, 0.6, 1.0f); // warm sunlight
    float lightIntensity = 2.0; //
    
    float4 baseColor       = materialProperties.baseColorFactor;
    float  roughnessFactor = materialProperties.roughnessFactor;
    float  metallicFactor  = materialProperties.metallicFactor;
    float3 emissiveFactor  = materialProperties.emissiveFactor;
    
    if ((materialProperties.materialFlags & HasMetallicRoughnessTex)  != 0)
    {
        float3 mrSample = pbrMetallicRoughnessTexture.Sample(gSampler, input.texcoord0).rgb;

        roughnessFactor = mrSample.g * roughnessFactor;
        metallicFactor = mrSample.b * metallicFactor;
    }
    
    
    if ((input.flags & HasTexCoord) != 0)
    {
        float2 uv = input.texcoord0;
        baseColor *= pbrBaseColorTexture.Sample(gSampler, uv);
    }
    
    if ((input.flags & HasNormal) != 0)
    {
        float3 N    = normalize(input.normal);
        float3 V    = normalize(sceneConstants.g_cameraPosition.xyz - input.worldPosition.xyz);
        float3 L    = normalize(-lightDir.xyz);
        float3 H    = normalize(V + L);
        float NdotL = saturate(dot(N, L));
        float NdotV = saturate(dot(N, V));
        float3 F0   = lerp(float3(0.04f, 0.04f, 0.04f), baseColor.rgb, metallicFactor);
        
        float  NDF = DistributionGGX(N, H, roughnessFactor);
        float  G   = GeometrySmith(N, V, L, roughnessFactor);
        float3 F   = FresnelSchlick(saturate(dot(H, V)), F0);
        
        float3 specular = (NDF * G * F) / (4.0 * NdotV * NdotL + 0.001);
        
        // Energy conservation
        float3 kS = F;
        float3 kD = (1.0 - kS) * (1.0 - metallicFactor);
        
        float3 diffuse = kD * baseColor.rgb / 3.14159265;
        
        baseColor = float4((diffuse + specular) * NdotL, baseColor.a);
        
        baseColor = float4(diffuse * NdotL, baseColor.a);

    }
    
    baseColor = baseColor * lightColor * lightIntensity;
    
    if ((materialProperties.materialFlags & HasEmissiveTexture) != 0)
    {
        float4 emissiveColor = emissiveTexture.Sample(gSampler, input.texcoord0) * float4(emissiveFactor, 1.0f);
        baseColor += emissiveColor;
    }
    
    if ((sceneConstants.renderFlags & RenderFlagsUsePBR) == 0)
    {
        baseColor = materialProperties.baseColorFactor;
        if ((input.flags & HasTexCoord) != 0)
        {
            float2 uv = input.texcoord0;
            baseColor *= pbrBaseColorTexture.Sample(gSampler, uv);
        }
    }

    //@todo force a=1 for opaque
        return float4(baseColor);

}
