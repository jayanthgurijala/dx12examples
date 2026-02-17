#include "Raytracing.hlsli"
#include "CameraBuffer.hlsli"

Texture2D gTexture    : register(t0);
SamplerState gSampler : register(s0);
RaytracingAccelerationStructure Scene : register(t3, space0);
RWTexture2D<float4> UAVOutput : register(u0, space0);

StructuredBuffer<uint> indexBuffer : register(t1, space0);
StructuredBuffer<float2> uvVbBuffer  : register(t2, space0);


// Generate a ray in world space for a camera pixel corresponding to an index from the dispatched 2D grid.
inline void GenerateCameraRay(uint2 index, out float3 origin, out float3 direction)
{
    //float4x4 projectionToWorld_T =
    //{
    //    { 0.55604, 0.239525, -0.419169, 0 },
    //    { -1.80351e-08, 0.359638, 0.205507, 0 },
    //    { 3.472, -1.984, 3.472, -0.992 },
    //    { -2.84439, 1.62537, -2.84439, 1 }
    //};

    float2 xy = index + 0.5f; // center in the middle of the pixel.
    float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

    // Invert Y for DirectX-style coordinates.
    screenPos.y = -screenPos.y;

    // Unproject the pixel coordinate into a ray.
    float4 world = mul(float4(screenPos, 0, 1), g_vpInv);

    world.xyz /= world.w;
    origin = g_cameraPosition.xyz;
    direction = normalize(world.xyz - origin);
}

[shader("raygeneration")]
void MyRaygenShader()
{
    float3 direction;
    float3 origin;
    GenerateCameraRay(DispatchRaysIndex().xy, origin, direction);
    RayPayload payload = { float4((direction + 0.5f) / 2, 1.0f) };
    RayDesc ray;
    ray.Direction = direction;
    ray.Origin    = origin;
    ray.TMin      = 0.001;
    ray.TMax      = 100;
    
    TraceRay(Scene, 0, 0xFF, 0, 0, 0, ray, payload);
    
     // Write the raytraced color to the output texture.
    UAVOutput[DispatchRaysIndex().xy] = payload.color;
}

[shader("closesthit")]
void MyClosestHitShader(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    
    
    ///index buffer [1 0]
    //              [3 2]
    uint primitiveIndexInGeom = PrimitiveIndex();
    uint numIndicesPerPrim = 3;
    uint numBytesPerIndex = 2;
    uint byteOffsetIntoIndexBuffer = primitiveIndexInGeom * numIndicesPerPrim * numBytesPerIndex;
    
    //16-bit buffer Index?
    uint indexAccessedAs2Byte = byteOffsetIntoIndexBuffer / 2;
    
    //32-bit buffer?
    uint indexAccessedAs4Byte = byteOffsetIntoIndexBuffer / 4;

    
    uint3 indices;
    if (indexAccessedAs2Byte % 2 == 0)
    {
        indices.x = indexBuffer[indexAccessedAs4Byte] & 0x0000FFFF;
        indices.y = (indexBuffer[indexAccessedAs4Byte] >> 16) & 0x0000FFFF;
        indices.z = indexBuffer[indexAccessedAs4Byte + 1] & 0x0000FFFF;
    }
    else
    {
        indices.x = (indexBuffer[indexAccessedAs4Byte] >> 16) & 0x0000FFFF;
        indices.y = indexBuffer[indexAccessedAs4Byte + 1] & 0x0000FFFF;
        indices.z = (indexBuffer[indexAccessedAs4Byte + 1] >> 16) & 0x0000FFFF;
    }
    
   
    float2 uv0 = uvVbBuffer[indices.x];
    float2 uv1 = uvVbBuffer[indices.y];
    float2 uv2 = uvVbBuffer[indices.z];
    
    float b0 = (1 - attr.barycentrics.x - attr.barycentrics.y);
    float b1 = attr.barycentrics.x;
    float b2 = attr.barycentrics.y;
    
    float2 uvInterpolated =b1 * uv1 + b2 * uv2 + b0 * uv0;
    payload.color = gTexture.SampleLevel(gSampler, uvInterpolated, 0);
}

[shader("miss")]
void MyMissShader(inout RayPayload payload)
{
    //payload.color = float4(0.8f, 0.1f, 0.1f, 1.0f);
}