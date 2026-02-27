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

float4 PSMain(VSOutput_2 input) : SV_TARGET
{
    float4 baseColor = baseColorFactor;
    
    float3 lightDir = float3(0, 1, 0);
    
    float3 N = normalize(input.normal);
    float3 V = normalize(g_cameraPosition.xyz - input.worldPosition.xyz);
    float3 L = normalize(lightDir.xyz);

    float  NdotL = saturate(dot(N, L));
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), baseColorFactor.rgb, metallicFactor);
    float3 F = FresnelSchlick(saturate(dot(N, V)), F0);
    
    float3 diffuse = baseColorFactor.rgb * (1.0 - F);
    float3 specular = F;
    
    float3 color = (diffuse + specular) * NdotL + emissiveFactor;

    return float4(color, 1.0f);

}
