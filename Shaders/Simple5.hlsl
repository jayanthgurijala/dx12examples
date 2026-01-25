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
    
    //@todo conver this to model space
    output.normal    = input.normal;
   
    output.texcoord0 = input.texcoord0;
    output.texcoord1 = input.texcoord1;
    output.texcoord2 = input.texcoord2;
    
    return output;
}

float4 PSMain(PSInput_5 input) : SV_TARGET
{
    float3 albedo = pow(abs(gTexture.Sample(gSampler, input.texcoord0).xyz), 2.2);
    
    //needs a ton of calculations
    float3 color = albedo;
    
    color = pow(color, 1.0 / 2.2);
    
    return float4(color, 1.0f);

}