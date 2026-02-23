struct MaterialData
{
    //Grouped as 16 bytes chunks
    
    float4 baseColorFactor; 

    
    float3 emissiveFactor;
    float normalScale;

    float metallicFactor;
    float roughnessFactor;
    float occlusionStrength;
    float alphaCutoff;

    //(0=Opaque,1=Mask,2=Blend)
    uint alphaMode;
    float3 padding;
};