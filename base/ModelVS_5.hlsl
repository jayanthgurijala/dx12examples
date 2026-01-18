#include "SimpleVSPSInterface.hlsli"

cbuffer MVP : register(b0)
{
    float4x4 g_MVP;
}

VSOutputPrimitive main(VSInputPrimitive input)
{
    VSOutputPrimitive output;
    
    output.position = mul(float4(input.position, 1.0f), g_MVP);
    output.texcoord0 = input.texcoord0;
    
    return output;
}