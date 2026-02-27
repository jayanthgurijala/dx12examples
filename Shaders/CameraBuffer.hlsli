


#define PI 3.14159265359

cbuffer MVP : register(b0)
{
    float4x4 g_modelMatrixT;
    float4x4 g_mvpT;
    float4x4 g_vpInv;
    float4x4 g_normalMatrix;
    float4   g_cameraPosition;
}

static const uint HasBaseColorTexture = 1 << 0;
static const uint HasNormalTexture = 1 << 1;
static const uint HasMetallicRoughnessTex = 1 << 2;
static const uint HasOcclusionTexture = 1 << 3;
static const uint HasEmissiveTexture = 1 << 4;
static const uint AlphaModeMask = 1 << 5;
static const uint AlphaModeBlend = 1 << 6;
static const uint DoubleSided = 1 << 7;

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0f - cosTheta, 5.0f);

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
   
    uint flags;
    float3 padding;
};