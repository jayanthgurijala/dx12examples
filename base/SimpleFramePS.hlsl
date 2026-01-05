#include "SimpleVSPSInterface.hlsli"

//needs a sampler and srv
Texture2D gTexture    : register(t0);
SamplerState gSampler : register(s0);

float4 main(PSInput input) : SV_TARGET
{
    return gTexture.Sample(gSampler, input.texcoord);
}