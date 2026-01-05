#include "SimpleVSPSInterface.hlsli"


VSOutput main(VSInput input)
{
    VSOutput output;
    
    output.position = float4(input.position, 1.0f);
    output.color    = input.color;
    output.texcoord = input.texcoord;

	return output;
}