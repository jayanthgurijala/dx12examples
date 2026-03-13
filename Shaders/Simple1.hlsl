#include "SimpleVSPSInterface.hlsli"
#include "CameraBuffer.hlsli"

VSOutput_1 VSMain( VSInput_1 input )
{
    VSOutput_1 output;
    
    //if transposed on CPU then hlsl reads this as row-major
    //in row-major, v' = v * M
    COMPUTE_POSITION(input.position, output.worldPosition, output.position);

    return output;
}

float4 PSMain(PSInput_1 input) : SV_TARGET
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}
