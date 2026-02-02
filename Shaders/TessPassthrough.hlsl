#include "SimpleVSPSInterface.hlsli"
#include "CameraBuffer.hlsli"


VSOutput_5_Tess VSMain(VSInput_5 input)
{
    VSOutput_5_Tess output = (VSOutput_5_Tess) 0;
    output.position = float4(input.position, 1.0);
    output.normal = input.normal;
    output.texcoord0 = input.texcoord0;
    output.texcoord1 = input.texcoord1;
    output.texcoord2 = input.texcoord2;
    return output;
}


HSConstantsTriOutput HSConstantFunc(InputPatch<VSOutput_5_Tess, 3> patch, uint patchId : SV_PrimitiveID)
{
    HSConstantsTriOutput triPatchConstants;
    triPatchConstants.TessLevelInner = 1;
    triPatchConstants.TessLevelOuter[0] = 1;
    triPatchConstants.TessLevelOuter[1] = 1;
    triPatchConstants.TessLevelOuter[2] = 1;
    return triPatchConstants;
}

[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("HSConstantFunc")]
HSOutput_5 HSMain(
    InputPatch<VSOutput_5_Tess, 3> patch,
    uint i : SV_OutputControlPointID,
    uint patchId : SV_PrimitiveID)
{
    HSOutput_5 output;

    // Pass through control point position
    output.position = patch[i].position;
    output.texcoord0 = patch[i].texcoord0;

    return output;
}

[domain("tri")]
DSOutput_5 DSMain(HSConstantsTriOutput tessFactors, float3 TessCoord : SV_DomainLocation, const OutputPatch<HSOutput_5, 3> patch)
{
    DSOutput_5 output = (DSOutput_5) 0;
    output.position = (TessCoord.x * patch[0].position) +
                      (TessCoord.y * patch[1].position) +
                      (TessCoord.z * patch[2].position);
    
    output.position = mul(output.position, g_mvpT);
    output.worldPosition = mul(output.position, g_modelMatrixT);
    //output.normal = normalize(mul(input.normal, (float3x3) g_normalMatrix));
   
    output.texcoord0 = patch[0].texcoord0 * TessCoord.x + patch[1].texcoord0 * TessCoord.y + patch[2].texcoord0 * TessCoord.z;
    //output.texcoord1 = input.texcoord1;
    //output.texcoord2 = input.texcoord2;
    return output;
}

//needs a sampler and srv
Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

float4 PSMain(DSOutput_5 input) : SV_TARGET
{
    float3 sampledColor = gTexture.Sample(gSampler, input.texcoord0).xyz;
    return float4(sampledColor, 1.0f);
};





