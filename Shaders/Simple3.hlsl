#include "SimpleVSPSInterface.hlsli"
#include "CameraBuffer.hlsli"


VSOutput_3 VSMain(VSInput_3 input)
{
    VSOutput_3 output;
    
    //if transposed on CPU then hlsl reads this as row-major
    //in row-major, v' = v * M
    output.position = mul(float4(input.position, 1.0f), g_mvpT);
    output.normal = normalize(mul(input.normal, (float3x3) g_normalMatrix));
    output.position /= output.position.w;
    output.texcoord = input.texcoord;
    
    return output;
}


float4 PSMain(PSInput_3 input) : SV_TARGET
{
    return float4(input.normal, 1.0f);

}
