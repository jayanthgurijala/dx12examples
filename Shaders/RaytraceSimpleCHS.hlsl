#include "Raytracing.hlsli"
#include "CameraBuffer.hlsli"

//Materials t0 to t4
Texture2D gTexture    : register(t0, space3); 

//Ray tracing specific
StructuredBuffer<float2> uvVbBuffer : register(t5, space3); 
StructuredBuffer<float3> posVbBuffer : register(t6, space3); 
StructuredBuffer<uint> indexBuffer    : register(t7, space3);

RaytracingAccelerationStructure Scene : register(t0, space2);
RWTexture2D<float4> UAVOutput : register(u0, space0);

SamplerState gSampler : register(s0);


// Generate a ray in world space for a camera pixel corresponding to an index from the dispatched 2D grid.
inline void GenerateCameraRay(uint2 index, out float3 origin, out float3 direction)
{
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
    
    TraceRay(Scene,
             0,
             0xFF,
             0,
             0,
            0,
            ray,
            payload);
    
     // Write the raytraced color to the output texture.
    UAVOutput[DispatchRaysIndex().xy] = payload.color;
}

uint3 GetHitTriangleIndices()
{
    uint3 indices;
    uint primitiveIndexInGeom = PrimitiveIndex();
    
    uint numIndicesPerPrim = 3;
    uint numBytesPerIndex = 2;
    uint byteOffsetIntoIndexBuffer = primitiveIndexInGeom * numIndicesPerPrim * numBytesPerIndex;
    
    //16-bit buffer Index?
    uint indexAccessedAs2Byte = byteOffsetIntoIndexBuffer / 2;
    
    //32-bit buffer?
    uint indexAccessedAs4Byte = byteOffsetIntoIndexBuffer / 4;
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
    
    return indices;
    
}

float CalculateRayConeUVFootPrint(float3 p[3], float2 uv[3])
{
    float rayHitT = RayTCurrent();
    float tanValue = tan(g_fovInRadians * 0.5f);
    float rayConeRadius = rayHitT * tanValue;
    float spread = rayConeRadius / DispatchRaysDimensions().x;

    float3 dp1 = p[1] - p[0];
    float3 dp2 = p[2] - p[0];
    
    float lendp1 = length(dp1);
    float lendp2 = length(dp2);
    
    float2 duv1 = uv[1] - uv[0];
    float2 duv2 = uv[2] - uv[0];
    float lenduv1 = length(duv1);
    float lenduv2 = length(duv2);
    
    float dduv1 = lenduv1 / lendp1;
    float dduv2 = lenduv2 / lendp2;
    
    float maxddu = max(dduv1, dduv2);
    
    float uvFootPrint = spread * maxddu;

    uint width, height;
    gTexture.GetDimensions(width, height);
    
    float rho = uvFootPrint * max(width, height);
    
    return rho;
    
}

[shader("closesthit")]
void CHSBaseColorTexturing(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{    
    uint3 indices = GetHitTriangleIndices();
    
    float3 p[3];
    float2 uv[3];
    
    p[0] = posVbBuffer[indices.x];
    p[1] = posVbBuffer[indices.y];
    p[2] = posVbBuffer[indices.z];
 
    
    uv[0] = uvVbBuffer[indices.x];
    uv[1] = uvVbBuffer[indices.y];
    uv[2] = uvVbBuffer[indices.z];
    
   
    
    float b0 = (1 - attr.barycentrics.x - attr.barycentrics.y);
    float b1 = attr.barycentrics.x;
    float b2 = attr.barycentrics.y;
    
    float2 uvInterpolated =b1 * uv[1] + b2 * uv[2] + b0 * uv[0];
    
    float rho = CalculateRayConeUVFootPrint(p, uv);
    float mipLevel = log2(rho);
    
    //payload.color = float4(mipLevel / 11.0, 0, 0, 1);
    payload.color = gTexture.SampleLevel(gSampler, uvInterpolated, mipLevel);
}

[shader("anyhit")]
void AHSAlphaCutOff(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    uint3 indices = GetHitTriangleIndices();

    float3 p[3];
    float2 uv[3];
    
    p[0] = posVbBuffer[indices.x];
    p[1] = posVbBuffer[indices.y];
    p[2] = posVbBuffer[indices.z];
 
    
    uv[0] = uvVbBuffer[indices.x];
    uv[1] = uvVbBuffer[indices.y];
    uv[2] = uvVbBuffer[indices.z];
    
    float b0 = (1 - attr.barycentrics.x - attr.barycentrics.y);
    float b1 = attr.barycentrics.x;
    float b2 = attr.barycentrics.y;
    
    float2 uvInterpolated = b1 * uv[1] + b2 * uv[2] + b0 * uv[0];
    
    float rho = CalculateRayConeUVFootPrint(p, uv);
    float mipLevel = log2(rho);
    float4 color = gTexture.SampleLevel(gSampler, uvInterpolated, 0);
    
    if (color.a < 0.5f)
    {
        IgnoreHit();
    }
    else
    {
        AcceptHitAndEndSearch();
    }
    
   
}

[shader("closesthit")]
void CHSNormalMapping(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    payload.color = float4(1.0, 1.0, 1.0, 1.0);
}

[shader("miss")]
void MyMissShader(inout RayPayload payload)
{
    payload.color = float4(0.5f, 0.5f, 0.5f, 1.0f);
}