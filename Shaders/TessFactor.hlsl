#include "CameraBuffer.hlsli"
#include "SimpleVSPSInterface.hlsli"

cbuffer RootConstant : register(b1)
{
    uint tessLevel; // single 32-bit constant
};




