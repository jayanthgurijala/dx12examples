#include "SimpleVSPSInterface.hlsli"
#include "CameraBuffer.hlsli"


VSOutput_3 VSMain(VSInput_3 input)
{
    VSOutput_3 output;
    
    //if transposed on CPU then hlsl reads this as row-major
    //in row-major, v' = v * M
    output.position      = mul(float4(input.position, 1.0f), g_mvpT);
    output.position      /= output.position.w;
    
    output.worldPosition = mul(float4(input.position, 1.0f), g_modelMatrixT);
    
    output.normal        = input.normal;
    output.texcoord      = input.texcoord;
    
    return output;
}


SamplerState gSampler : register(s0);

Texture2D pbrBaseColorTexture : register(t0);
Texture2D pbrMetallicRoughnessTexture : register(t1);
Texture2D normalTexture : register(t2);
Texture2D occlusionTexture : register(t3);
Texture2D emissiveTexture : register(t4);

    

float4 PSMain(VSOutput_3 input) : SV_TARGET
{
    
    float3 lightDir = float3(0, 4, 4);
    
    
    float3 N = normalize(input.normal);
    float3 L = normalize(lightDir.xyz);
    
    float4 baseColor = baseColorFactor;
    
    if (flags & HasBaseColorTexture)
    {
        baseColor *= pbrBaseColorTexture.Sample(gSampler, input.texcoord);
    }
    
    return float4(baseColor);

}
