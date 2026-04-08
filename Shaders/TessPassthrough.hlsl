#include "SimpleVSPSInterface.hlsli"
#include "CameraBuffer.hlsli"

//needs a sampler and srv
Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);






float4 PSMain(DSOutput_3 input) : SV_TARGET
{
    float3 sampledColor = gTexture.Sample(gSampler, input.texcoord0).xyz;
    return float4(sampledColor, 1.0f);
};







