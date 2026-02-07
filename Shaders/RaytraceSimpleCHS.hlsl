#include "Raytracing.hlsli"

RaytracingAccelerationStructure Scene : register(t1, space0);
RWTexture2D<float4> UAVOutput : register(u0, space0);

typedef BuiltInTriangleIntersectionAttributes MyAttributes;

[shader("raygeneration")]
void MyRaygenShader()
{
    RayDesc ray;
    
    //@todo Orthographic projection since we're raytracing in screen space - change this
    float3 rayDir    = float3(0, 0, 1);
    float3 rayOrigin = float3((float2) DispatchRaysIndex() / (float2) DispatchRaysDimensions(), 0.0f);
    RayPayload payload = { float4(0,0,0,0) };
    
    ray.Direction = rayDir;
    ray.Origin    = rayOrigin;
    ray.TMin      = 0.001;
    ray.TMax      = 100;
    
    TraceRay(Scene,                                                 //TLAS
            RAY_FLAG_CULL_BACK_FACING_TRIANGLES,                    //Flags
            ~0,                                                     //InstanceInclusionMask
             0,                                                     //RayContributionToHitGroupIndex
             1,                                                     //MultiplierForGeometryContributionToHitGroupIndex
             0,                                                     //Miss Shader Index
             ray,                                                   
             payload);
    
     // Write the raytraced color to the output texture.
    UAVOutput[DispatchRaysIndex().xy] = payload.color;
}

[shader("closesthit")]
void MyClosestHitShader(inout RayPayload payload, in MyAttributes attr)
{
    payload.color = float4(0.2f, 0.5f, 0.8f, 1.0f);
}

[shader("miss")]
void MyMissShader(inout RayPayload payload)
{
    payload.color = float4(0.8f, 0.1f, 0.1f, 1.0f);
}