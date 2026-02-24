#include "CameraBuffer.hlsli"
#include "SimpleVSPSInterface.hlsli"

cbuffer RootConstant : register(b1)
{
    float tessLevel;
};

VSOutput_2_Tess VSMain(VSInput_2 input)
{
    VSOutput_2_Tess output = (VSOutput_2_Tess) 0;
    output.position = float4(input.position, 1.0);
    output.normal = input.normal;
    return output;
}

HSConstantsTriOutput ConstantsHS(InputPatch<VSOutput_2_Tess, 3> patch, uint InvocationID : SV_PrimitiveID)
{
    HSConstantsTriOutput output = (HSConstantsTriOutput) 0;
    output.TessLevelOuter[0] = tessLevel;
    output.TessLevelOuter[1] = tessLevel;
    output.TessLevelOuter[2] = tessLevel;
    output.TessLevelInner = tessLevel;
    return output;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("ConstantsHS")]
[maxtessfactor(20.0f)]
HSOutput_2 HSMain(InputPatch<VSOutput_2_Tess, 3> patch, uint i : SV_OutputControlPointID)
{
    HSOutput_2 output = (HSOutput_2) 0;

    output.position = patch[i].position;
    output.normal = patch[i].normal;

    return output;
}

float3 ProjectToPlane(float3 vec, float3 normal)
{
    return vec - dot(vec, normal) * normal;
}

[domain("tri")]
DSOutput_2 DSMain(HSConstantsTriOutput input, const OutputPatch<HSOutput_2, 3> patch,
    float3 bary : SV_DomainLocation
)
{
    DSOutput_2 output = (DSOutput_2) 0;
    
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
    
    output.position = mul(output.position, g_mvpT);
    output.worldPosition = mul(output.position, g_modelMatrixT);

    return output;
}


//needs a sampler and srv
Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

float4 PSMain(DSOutput_2 input) : SV_TARGET
{
    float4 sampledColor = float4(input.normal, 1.0f);
    return sampledColor;
};




