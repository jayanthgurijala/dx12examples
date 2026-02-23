#include "SimpleVSPSInterface.hlsli"
#include "CameraBuffer.hlsli"


VSOutput_2 VSMain(VSInput_2 input)
{
    VSOutput_2 output;
    
    //if transposed on CPU then hlsl reads this as row-major
    //in row-major, v' = v * M
    output.position = mul(float4(input.position, 1.0f), g_mvpT);
    output.normal = normalize(mul(input.normal, (float3x3) g_normalMatrix));
    output.position /= output.position.w;
    
    return output;
}


float4 PSMain(PSInput_2 input) : SV_TARGET
{   
    return float4(input.normal, 1.0f);

}
