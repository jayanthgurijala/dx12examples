


#define PI 3.14159265359

cbuffer MVP : register(b0)
{
    float4x4 g_modelMatrixT;
    float4x4 g_mvpT;
    float4x4 g_vpInv;
    float4x4 g_normalMatrix;
    float4   g_cameraPosition;
}

cbuffer MaterialData : register(b1)
{
    float4 baseColorFactor;
    
    float3 emissiveFactor;
    float  metallicFactor;
    
    float roughnessFactor;
    float normalScale;
    float occlusionStrength;
    float alphaCutoff;
   
    uint alphaMode;
    uint flags;
    float2 padding;
};