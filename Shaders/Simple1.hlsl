#include "SimpleVSPSInterface.hlsli"
#include "CameraBuffer.hlsli"

VSOutput_1 VSMain( VSInput_1 input )
{
    VSOutput_1 output;
    float4x4 mvp = mul(g_modelMatrixT, g_viewProjT);
    output.position = mul(float4(input.position, 1.0f), g_mvpT);
    return output;
}


float4 PSMain(PSInput_1 input) : SV_TARGET
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}