#include "SimpleVSPSInterface.hlsli"


VSOutputSimpleFrameComposition main(VSInputSimpleFrameComposition input)
{
    VSOutputSimpleFrameComposition output;
    
    output.position = float4(input.position, 1.0f);
    output.texcoord = input.texcoord;

	return output;
}