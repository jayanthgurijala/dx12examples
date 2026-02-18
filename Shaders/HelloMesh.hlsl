#include "CameraBuffer.hlsli"

struct VSOutput
{
    float4 position  : SV_Position;
    float3 normal    : NORMAL;
    float2 uv        : TEXCOORD0;
    float4 color     : COLOR0;
};

struct Vertex
{
    float3 position;
};

StructuredBuffer<uint> indexBuffer : register(t1);
StructuredBuffer<Vertex> posVb : register(t2);
StructuredBuffer<float2> uvVbBuffer : register(t3, space0);


[NumThreads(64, 1, 1)]
[OutputTopology("triangle")]
void MSMain(uint3 gid       : SV_GroupID,
          uint3 gtid        : SV_GroupThreadID,
          out indices uint3 tris[64], //triangles or lines //first thread tri[0]   gid = 0, gtid = 0
          out vertices VSOutput vertices[192]) //first thread writes v[0] v[1] v[2]
{
    /// Let us output 64 triangles and 192 vertices.
    /// It is very non-optimal as there is 1-1 mapping between index and vertex
    /// Typically we want to reuse overlapping vertices, but this is a simple learning example.
    /// @todo Learn how to optimize this
    uint numTrianglesToOutputByThreadGroup = 64;
    uint numVerticesPerTriangle = 3;
    uint numVerticesToOutputByThreadGroup = numTrianglesToOutputByThreadGroup * numVerticesPerTriangle;
    
    ///SetMeshOutputCounts(numVertices, numPrimitives) => (192, 64)  
    ///@note 64 * 3 = 192
    SetMeshOutputCounts(numVerticesToOutputByThreadGroup, numTrianglesToOutputByThreadGroup); //-> space to reserve?
 
    if (gtid.x < numTrianglesToOutputByThreadGroup) 
    {
        //Each thread writes 1 index uint3 and 3 vertices => 64 uint3 and 192 vertices 
        uint groupPrimitiveStartIndex       = numTrianglesToOutputByThreadGroup * gid.x;
        uint threadGroupPrimitiveStartIndex = groupPrimitiveStartIndex + gtid.x;
        uint numIndicesPerPrimitive = 3;
        uint numBytesPerIndex = 2;
        uint threadGroupOffsetIntoIndexBufferInBytes = threadGroupPrimitiveStartIndex * numIndicesPerPrimitive * numBytesPerIndex;
        
        
        uint indexAccessedAs2ByteBuffer = threadGroupOffsetIntoIndexBufferInBytes / 2;
        uint indexAccessedAs4ByteBuffer = threadGroupOffsetIntoIndexBufferInBytes / 4;
        
        uint3 indexData = uint3(0, 0, 0);
        
        if (indexAccessedAs2ByteBuffer % 2 == 0)
        {
            uint index_1_0 = indexBuffer[indexAccessedAs4ByteBuffer];
            uint index_x_2 = indexBuffer[indexAccessedAs4ByteBuffer + 1];
            indexData.x = index_1_0 & 0xFFFF; 
            indexData.y = (index_1_0 >> 16) & 0xFFFF; 
            indexData.z = index_x_2 & 0xFFFF; 
        }
        else
        {
            uint index_0_x = indexBuffer[indexAccessedAs4ByteBuffer];
            uint index_2_1 = indexBuffer[indexAccessedAs4ByteBuffer + 1];
            indexData.x = (index_0_x >> 16) & 0xFFFF; 
            indexData.y = (index_2_1) & 0xFFFF; 
            indexData.z = (index_2_1 >> 16) & 0xFFFF;
        }
        
        uint3 vertexIndex;
        vertexIndex.x = gtid.x * 3;
        vertexIndex.y = gtid.x * 3 + 1;
        vertexIndex.z = gtid.x * 3 + 2;
        
        tris[gtid.x] = vertexIndex;
        
        float4 vertexData[3];
        vertexData[0] = float4(posVb[indexData.x].position, 1.0f);
        vertexData[1] = float4(posVb[indexData.y].position, 1.0f);
        vertexData[2] = float4(posVb[indexData.z].position, 1.0f);
        
        vertices[vertexIndex.x].position = mul(vertexData[0], g_mvpT);
        vertices[vertexIndex.y].position = mul(vertexData[1], g_mvpT);
        vertices[vertexIndex.z].position = mul(vertexData[2], g_mvpT);
        
        vertices[vertexIndex.x].uv = uvVbBuffer[indexData.x];
        vertices[vertexIndex.y].uv = uvVbBuffer[indexData.y];
        vertices[vertexIndex.z].uv = uvVbBuffer[indexData.z];

    }
}



Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

float4 PSMain(VSOutput input) : SV_TARGET
{
 
    
    float4 sampledColor = float4(gTexture.Sample(gSampler, input.uv).xyz, 1.0f);
    return sampledColor;

}


