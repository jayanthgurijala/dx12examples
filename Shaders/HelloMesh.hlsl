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


[NumThreads(128, 1, 1)]
[OutputTopology("triangle")]
void MSMain(uint3 gid       : SV_GroupID,
          uint3 gtid        : SV_GroupThreadID,
          out indices uint3 tris[42],
          out vertices VSOutput vertices[126])
{
    // 64 vertices and 126 indices uint3 indices
    // Each thread will write 3 indices
    SetMeshOutputCounts(126, 42); 

    //Need to output verticces and index.
    //Use groupID and threadGroupID to determine which vertex and index to output.
    //uint indexBufferOffset = gid.x * 126 + gtid.x; // Calculate the offset for the index buffer based on the group and thread
    
    //bound index buffer is 16-bit, but we access 32-bit at a time
    if (gtid.x < 42) //each thread group writes 2 indices and 2 vertices
    {
        //but buffer is packed with 2 16-bit indices 
        uint indexToAccess = gid.x * 126 + gtid.x*3; 
        uint indesc32ToReadFrom = indexToAccess / 2;
        
        uint3 indexData = uint3(0, 0, 0);
        
        if (indexToAccess % 2 == 0)
        {
            uint index_1_2 = indexBuffer[indesc32ToReadFrom];
            uint index_2_3 = indexBuffer[indesc32ToReadFrom + 1];
            indexData.x = index_1_2 & 0xFFFF; // First index (lower 16 bits)
            indexData.y = (index_1_2 >> 16) & 0xFFFF; // Second index (upper 16 bits)
            indexData.z = index_2_3 & 0xFFFF; // Third index (lower 16 bits)
        }
        else
        {
            uint index_x_1 = indexBuffer[indesc32ToReadFrom];
            uint index_2_3 = indexBuffer[indesc32ToReadFrom + 1];
            indexData.x = (index_x_1 >> 16) & 0xFFFF; // Second index (upper 16 bits)
            indexData.y = (index_2_3) & 0xFFFF; // Third index (lower 16 bits)
            indexData.z = (index_2_3 >> 16) & 0xFFFF; // Third index (upper 16 bits)
        }
        
        tris[gtid.x] = indexData; // Write the triangle indices for this thread]
        
        vertices[gtid.x * 3].position     = mul(float4(posVb[indexData.x].position, 1.0f), g_mvpT);
        vertices[gtid.x * 3 + 1].position = mul(float4(posVb[indexData.y].position, 1.0f), g_mvpT);
        vertices[gtid.x * 3 + 2].position = mul(float4(posVb[indexData.z].position, 1.0f), g_mvpT);
        
        vertices[gtid.x * 3].color = float4(gid.x / 42, 0, 0, 1); // Red color for testing
        vertices[gtid.x * 3 + 1].color = float4(gid.x / 42, 0, 0, 1); // Red color for testing
        vertices[gtid.x * 3 + 2].color = float4(gid.x / 42, 0, 0, 1); // Red color for testing
    }
}


float4 PSMain(VSOutput input) : SV_TARGET
{
    // Just output the color for now, you can add lighting and texture sampling here
    return float4(1.0, 1.0, 1.0, 1.0); // Red color for testing)
}


