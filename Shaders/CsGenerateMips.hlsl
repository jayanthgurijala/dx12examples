Texture2D<float4>   SrcMip : register(t0);
RWTexture2D<float4> DstMip : register(u0);
cbuffer  TexInfo : register(b0)
{
    uint Width;
    uint Height;
}


// Thread group size
[numthreads(8, 8, 1)]
void GenerateMipCS(uint3 globalTid : SV_DispatchThreadID,
                   uint3 gid       : SV_GroupID,
                   uint3 localTid  : SV_GroupThreadID)
{
    if (globalTid.x > Width || globalTid.y > Height)
        return;
   //where to read from src, hence *2
    uint2 groupBase  = (gid.xy * 8) * 2;   
    uint2 threadBase = localTid.xy * 2;
    uint2 srcCoord   = groupBase + threadBase;
    
    float4 c0 = SrcMip.Load(uint3(srcCoord + uint2(0, 0), 0));
    float4 c1 = SrcMip.Load(uint3(srcCoord + uint2(0, 1), 0));
    float4 c2 = SrcMip.Load(uint3(srcCoord + uint2(1, 0), 0));
    float4 c3 = SrcMip.Load(uint3(srcCoord + uint2(1, 1), 0));
    
    DstMip[globalTid.xy] = (c0 + c1 + c2 + c3) * 0.25f;
    
}