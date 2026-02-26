#include "SimpleVSPSInterface.hlsli"
#include "CameraBuffer.hlsli"


VSOutput_2 VSMain(VSInput_2 input)
{
    VSOutput_2 output;
    
    //if transposed on CPU then hlsl reads this as row-major
    //in row-major, v' = v * M
    output.position = mul(float4(input.position, 1.0f), g_mvpT);
    output.position /= output.position.w;
    output.normal = normalize(mul(input.normal, (float3x3) g_normalMatrix));
    output.worldPosition = mul(float4(input.position, 1.0f), g_modelMatrixT);
 
    return output;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0f - cosTheta, 5.0f);

}

float4 PSMain(VSOutput_2 input) : SV_TARGET
{
    float3 lightDir = float3(0, 0, 1);
    
    float3 N = normalize(input.normal);
    float3 V = normalize(g_cameraPosition - input.worldPosition);
    float3 L = normalize(lightDir);

    float  NdotL = saturate(dot(N, L));
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), baseColorFactor.rgb, metallicFactor);
    float F = FresnelSchlick(saturate(dot(N, V)), F0);
    
    float3 diffuse = baseColorFactor.rgb * (1.0 - F);
    float3 specular = F;
    
    float3 color = (diffuse + specular) * NdotL;

    return float4(color, 1.0f);

}
