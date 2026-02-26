#include "SimpleVSPSInterface.hlsli"
#include "CameraBuffer.hlsli"


VSOutput_5 VSMain(VSInput_5 input)
{
    VSOutput_5 output;
    
    //if transposed on CPU then hlsl reads this as row-major
    //in row-major, v' = v * M
    output.position = mul(float4(input.position, 1.0f), g_mvpT);
    output.worldPosition = mul(float4(input.position, 1.0f), g_modelMatrixT);
    output.normal = normalize(mul(input.normal, (float3x3) g_normalMatrix));
   
    output.texcoord0 = input.texcoord0;
    output.texcoord1 = input.texcoord1;
    output.texcoord2 = input.texcoord2;
    
    output.position /= output.position.w;
    
    return output;
}


//needs a sampler and srv
Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

float4 PSMain(PSInput_5 input) : SV_TARGET
{   
        float3 sampledColor = gTexture.Sample(gSampler, input.texcoord0).xyz;   
        return float4(sampledColor, 1.0f);
}
