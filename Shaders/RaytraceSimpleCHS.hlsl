#include "Raytracing.hlsli"
#include "CameraBuffer.hlsli"

RaytracingAccelerationStructure Scene : register(t1, space0);
RWTexture2D<float4> UAVOutput : register(u0, space0);

typedef BuiltInTriangleIntersectionAttributes MyAttributes;

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
    
    TraceRay(Scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, ray, payload);
    
     // Write the raytraced color to the output texture.
    UAVOutput[DispatchRaysIndex().xy] = payload.color;
}

[shader("closesthit")]
void MyClosestHitShader(inout RayPayload payload, in MyAttributes attr)
{
    payload.color = float4(0.8f, 0.8f, 0.8f, 1.0f);
}

[shader("miss")]
void MyMissShader(inout RayPayload payload)
{
    //payload.color = float4(0.8f, 0.1f, 0.1f, 1.0f);
}