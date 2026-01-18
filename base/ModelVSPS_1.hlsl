#include "SimpleVSPSInterface.hlsli"

cbuffer MVP : register(b0)
{
    float4x4 g_MVP;
}

VSOutput_1 vsmain( VSInput_1 input )
{
    VSOutput_1 output;
    output.position = mul(float4(input.position, 1.0f), g_MVP);
    return output;
}


float4 psmain(PSInputPrimitive input) : SV_TARGET
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}