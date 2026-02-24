#include "SimpleVSPSInterface.hlsli"
#include "CameraBuffer.hlsli"


VSOutput_3_Tess VSMain(VSInput_3 input)
{
    VSOutput_3_Tess output = (VSOutput_3_Tess) 0;
    output.position = float4(input.position, 1.0);
    output.normal = input.normal;
    output.texcoord0 = input.texcoord;
    return output;
}


HSConstantsTriOutput HSConstantFunc(InputPatch<VSOutput_3_Tess, 3> patch, uint patchId : SV_PrimitiveID)
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
HSOutput_3 HSMain(
    InputPatch<VSOutput_3_Tess, 3> patch,
    uint i : SV_OutputControlPointID,
    uint patchId : SV_PrimitiveID)
{
    HSOutput_3 output;

    // Pass through control point position
    output.position = patch[i].position;
    output.texcoord0 = patch[i].texcoord0;
    output.normal = patch[i].normal;
    return output;
}

[domain("tri")]
DSOutput_3 DSMain(HSConstantsTriOutput tessFactors, float3 TessCoord : SV_DomainLocation, const OutputPatch<HSOutput_3, 3> patch)
{
    DSOutput_3 output = (DSOutput_3) 0;
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

float4 PSMain(DSOutput_3 input) : SV_TARGET
{
    float3 sampledColor = gTexture.Sample(gSampler, input.texcoord0).xyz;
    return float4(sampledColor, 1.0f);
};







