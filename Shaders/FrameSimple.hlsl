#include "SimpleVSPSInterface.hlsli"

//needs a sampler and srv
Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

VSOutputSimpleFrameComposition VSMain(VSInputSimpleFrameComposition input)
{
    VSOutputSimpleFrameComposition output;
    
    output.position = float4(input.position, 1.0f);
    output.texcoord = input.texcoord;

	return output;
}


float4 PSMain(PSInputSimpleFrameComposition input) : SV_TARGET
{
    return gTexture.Sample(gSampler, input.texcoord);
}