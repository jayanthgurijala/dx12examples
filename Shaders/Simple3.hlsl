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
    output.normal        = normalize(mul(input.normal, (float3x3) g_normalMatrix));
    output.texcoord      = input.texcoord;

    
    return output;
}


float4 PSMain(VSOutput_3 input) : SV_TARGET
{
    float4 color = baseColorFactor;

    return color;   

}
