

#define PI 3.14159265359

cbuffer MVP : register(b0)
{
    float4x4 g_modelMatrixT;
    float4x4 g_mvpT;
    float4x4 g_normalMatrix;
    float4   g_cameraPosition;
}