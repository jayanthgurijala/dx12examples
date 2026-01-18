#include "SimpleVSPSInterface.hlsli"

cbuffer MVP : register(b0)
{
    float4x4 g_MVP;
}

VSOutputPrimitive VSMain(VSInputPrimitive input)
{
    VSOutputPrimitive output;
    
    output.position = mul(float4(input.position, 1.0f), g_MVP);
    output.texcoord0 = input.texcoord0;
    
    return output;
}

float4 PSMain(PSInputPrimitive input) : SV_TARGET
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}