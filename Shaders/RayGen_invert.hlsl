#include "Raytracing.hlsli"
#include "CameraBuffer.hlsli"

RaytracingAccelerationStructure Scene : register(t0, space2);
RWTexture2D<float4> UAVOutput : register(u0, space0);

inline RayPayload InitWithDirectionColor(float3 direction)
{
    RayPayload payload;
    float3 normalizedDirection = (direction + 0.5f) / 2; // Shift to [0, 1] range for visualization.
    float4 color = float4(normalizedDirection, 1.0f);
    
    payload.color = color;
    payload.currentRecursionDepth = 0;
    return payload;
}

inline RayDesc GetRay(float3 origin, float3 direction)
{
    RayDesc ray;
    ray.Direction = direction;
    ray.Origin = origin;
    ray.TMin = 0.001;
    ray.TMax = 100;
    return ray;
}


// Generate a ray in world space for a camera pixel corresponding to an index from the dispatched 2D grid.
inline void GenerateCameraRay(uint2 index, out float3 origin, out float3 direction)
{
    float2 xy = index + 0.5f; // center in the middle of the pixel.
    float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

    // Invert Y for DirectX-style coordinates.
    //screenPos.y = -screenPos.y;

    // Unproject the pixel coordinate into a ray.
    float4 world = mul(float4(screenPos, 0, 1), sceneConstants.g_vpInv);

    world.xyz /= world.w;
    origin = sceneConstants.g_cameraPosition.xyz;
    direction = normalize(world.xyz - origin);
}

inline void TraceRadianceRay(float3 origin, float3 direction, out RayPayload payload, uint recursionDepth)
{
    RayPayload payload_ = InitWithDirectionColor(direction);
    RayDesc ray = GetRay(origin, direction);
    
    uint missShaderIndex = 0;
    uint rayContrubitionToHitGroupIndex = 0;
    uint multiplierForGeometryContribution = 2;
    uint rayFlags = 0;
    uint instanceInclusionMask = 0xFF;

    //Level one
    payload_.currentRecursionDepth = recursionDepth + 1;
    TraceRay(Scene,
             rayFlags,
             instanceInclusionMask,
             rayContrubitionToHitGroupIndex,
             multiplierForGeometryContribution,
             missShaderIndex,
             ray,
             payload_);
    payload = payload_;
}

[shader("raygeneration")]
void MyRaygenShader_invert()
{
    float3 direction;
    float3 origin;
    RayPayload payload;
    GenerateCameraRay(DispatchRaysIndex().xy, origin, direction);
    TraceRadianceRay(origin, direction, payload, 0);

     // Write the raytraced color to the output texture.
    UAVOutput[DispatchRaysIndex().xy] = payload.color;
}


