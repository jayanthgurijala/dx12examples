#include "SimpleVSPSInterface.hlsli"
#include "CameraBuffer.hlsli"

VSOutput_1 VSMain( VSInput_1 input )
{
    VSOutput_1 output;
    
    //if transposed on CPU then hlsl reads this as row-major
    //in row-major, v' = v * M
    output.position     = mul(float4(input.position, 1.0f), g_mvpT);
    output.position     /= output.position.w;
    output.worldPosition = mul(float4(input.position, 1.0f), g_modelMatrixT);

    return output;
}

float4 PSMain(PSInput_1 input) : SV_TARGET
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}
