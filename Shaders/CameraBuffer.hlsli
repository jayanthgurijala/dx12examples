#define PI 3.14159265359

struct SceneConstants
{
    float4x4 g_viewProjT;
    
    float4x4 g_vpInv;
    
    float4 g_cameraPosition;
    
    float g_fovInRadians;
    float3 camBufferpadding;
    
    uint renderFlags;
    float3 padding;
};

cbuffer SceneCB : register(b0)
{
    SceneConstants sceneConstants;
}


//In ray tracing, this is baked into instance description
cbuffer WorldMatrix : register(b1)
{
    float4x4 g_modelMatrixT;
    float4x4 g_normalMatrix;
}

struct MaterialProperties
{
    float4 baseColorFactor;
    
    float3 emissiveFactor;
    float metallicFactor;
    
    float roughnessFactor;
    float normalScale;
    float occlusionStrength;
    float alphaCutoff;
   
    uint materialFlags;
    float3 padding;
};

cbuffer MaterialData : register(b0, space3)
{
    MaterialProperties materialProperties;
};

static const uint HasBaseColorTexture     = 1 << 0;
static const uint HasNormalTexture        = 1 << 1;
static const uint HasMetallicRoughnessTex = 1 << 2;
static const uint HasOcclusionTexture     = 1 << 3;
static const uint HasEmissiveTexture      = 1 << 4;
static const uint AlphaModeMask           = 1 << 5;
static const uint AlphaModeBlend          = 1 << 6;
static const uint DoubleSided             = 1 << 7;


static const uint RenderFlagsUsePBR       = 1 << 0;
static const uint RenderFlagsTessEnabled  = 1 << 1;

static const uint HasNormal   = 1 << 0;
static const uint HasTexCoord = 1 << 1;
static const uint HasTangent  = 1 << 2;



float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0f - cosTheta, 5.0f);

}
