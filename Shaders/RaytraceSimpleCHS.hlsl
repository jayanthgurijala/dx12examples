#include "Raytracing.hlsli"
#include "CameraBuffer.hlsli"

//Materials t0 to t4
Texture2D gTexture    : register(t0, space3); 

//Ray tracing specific
StructuredBuffer<float3> posVbBuffer     : register(t5, space3);
StructuredBuffer<float3> posNormalBuffer : register(t6, space3);
StructuredBuffer<float2> uvVbBuffer      : register(t7, space3); 
StructuredBuffer<uint> indexBuffer       : register(t8, space3);

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

inline RayPayload InitWithDirectionColor(float3 direction)
{
    RayPayload payload;
    float3 normalizedDirection = (direction + 0.5f) / 2; // Shift to [0, 1] range for visualization.
    float4 color               = float4(normalizedDirection, 1.0f);
    
    payload.color                 = color;
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

// Retrieve hit world position.
float3 HitWorldPosition()
{
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

inline bool TraceShadowRay(float3 origin, float3 direction, uint recursionDepth)
{
    if (recursionDepth > 1)
    {
        return false;
    }
    ShadowRayPayload payload;
    payload.isHit = false;
    
    
    uint missShaderIndex                   = 1;
    uint rayContrubitionToHitGroupIndex    = 1;
    uint multiplierForGeometryContribution = 2;
    uint rayFlags                          = 0;
    uint instanceInclusionMask             = 0xFF;

    //Level 2
    payload.currentRecursionDepth = recursionDepth + 1;
    TraceRay(Scene,
             rayFlags,
             instanceInclusionMask,
             rayContrubitionToHitGroupIndex,
             multiplierForGeometryContribution,
             missShaderIndex,
             GetRay(origin, direction),
             payload);

    return payload.isHit;
    

}

inline void TraceRadianceRay(float3 origin, float3 direction, out RayPayload payload, uint recursionDepth)
{
    RayPayload payload_ = InitWithDirectionColor(direction);
    RayDesc ray = GetRay(origin, direction);
    
    uint missShaderIndex                   = 0;
    uint rayContrubitionToHitGroupIndex    = 0;
    uint multiplierForGeometryContribution = 2;
    uint rayFlags                          = 0;
    uint instanceInclusionMask             = 0xFF;

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
void MyRaygenShader()
{
    float3 direction;
    float3 origin;
    RayPayload payload;
    GenerateCameraRay(DispatchRaysIndex().xy, origin, direction);
    TraceRadianceRay(origin, direction, payload, 0);

     // Write the raytraced color to the output texture.
    UAVOutput[DispatchRaysIndex().xy] = payload.color;
}

float2 InterpolateBarycentrics(float2 bary, float2 uv[3])
{
    float b0 = (1 - bary.x - bary.y);
    float b1 = bary.x;
    float b2 = bary.y;
    
    float2 uvInterpolated = b1 * uv[1] + b2 * uv[2] + b0 * uv[0];
    return uvInterpolated;
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

// Estimates the texture footprint (rho) for a ray hit using a ray cone approximation.
// This is useful for choosing the correct mip level when sampling a texture in ray tracing.
//
// p  - triangle vertex positions in world space (3 points of the hit triangle)
// uv - corresponding texture coordinates for those triangle vertices
//
// Returns:
// rho - an estimate of the texture footprint in texel space (used for mip selection)
float CalculateRayConeUVFootPrint(float3 p[3], float2 uv[3])
{
    // Distance along the ray where the hit occurred.
    // In ray tracing shaders, RayTCurrent() gives the parametric
    // distance to the closest intersection along the ray.
    float rayHitT = RayTCurrent();
    
    // Compute tangent of half the camera field of view.
    // This is used to estimate how much the ray cone spreads
    // as the ray travels through space.
    float tanValue = tan(g_fovInRadians * 0.5f);
    
    // Estimate the radius of the ray cone at the hit distance.
    // As the ray travels farther from the camera, the cone widens.
    float rayConeRadius = rayHitT * tanValue;
    
     // Convert cone radius into screen-space spread.
    // DispatchRaysDimensions().x is the width of the ray dispatch
    // (typically the screen width).
    //
    // This approximates how much world space area a single pixel ray covers.
    float spread = rayConeRadius / DispatchRaysDimensions().x;

    // Compute triangle edge vectors in world space.
    // These represent the geometric edges of the triangle.
    float3 dp1 = p[1] - p[0];
    float3 dp2 = p[2] - p[0];
    
    // Lengths of the geometric edges in world space.
    // Used to measure how large the triangle edges are physically.
    float lendp1 = length(dp1);
    float lendp2 = length(dp2);
    
     // Compute the equivalent edges in UV (texture) space.
    // These show how much the texture coordinates change across the triangle.
    float2 duv1 = uv[1] - uv[0];
    float2 duv2 = uv[2] - uv[0];
    
    // Lengths of the UV edges.
    // These indicate how much texture space corresponds to each triangle edge.
    float lenduv1 = length(duv1);
    float lenduv2 = length(duv2);
    
    // Compute the UV gradient relative to world-space edges.
    // Essentially: "how much UV changes per unit of world space".
    //
    // This approximates the texture coordinate derivative.
    float dduv1 = lenduv1 / lendp1;
    float dduv2 = lenduv2 / lendp2;
    
    // Use the maximum gradient as a conservative estimate.
    // This ensures the mip selection will not undersample the texture.
    float maxddu = max(dduv1, dduv2);
    
    // Convert the ray cone world-space spread into UV space spread.
    // This gives an estimate of how large the ray footprint is in UV coordinates.
    float uvFootPrint = spread * maxddu;

    uint width, height;
    gTexture.GetDimensions(width, height);
    
    // Convert UV footprint into texel footprint.
    // Multiplying by the larger texture dimension produces
    // an approximate footprint size in texel units.
    float rho = uvFootPrint * max(width, height);
    
    return rho;
}

inline void GetValues(uint3 indices, StructuredBuffer<float3> buffer, out float3 values[3])
{
    values[0] = buffer[indices.x];
    values[1] = buffer[indices.y];
    values[2] = buffer[indices.z];
}

inline void GetValues(uint3 indices,StructuredBuffer<float2> buffer, out float2 values[3])
{
    values[0] = buffer[indices.x];
    values[1] = buffer[indices.y];
    values[2] = buffer[indices.z];
}


[shader("closesthit")]
void CHSBaseColorTexturing(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{   
    //@todo put it in CBuffer
    float3 lightDirection = normalize(float3(0.5f, 1.0f, 0.5f));
    
    if (payload.currentRecursionDepth > 1)
    {
        payload.color = float4(1.0f, 0.0f, 1.0f, 1.0f); // Magenta for debugging excessive recursion.
        return;
    };
    
    uint3 indices = GetHitTriangleIndices();
    float3 triangleNormal = posNormalBuffer[indices[0]];
    
    float4 baseColor = materialProperties.baseColorFactor;
    if ((materialProperties.materialFlags & HasBaseColorTexture) != 0)
    {
       
        float3 p[3];
        float2 uv[3];
    
        GetValues(indices, posVbBuffer, p);
        GetValues(indices, uvVbBuffer, uv);
    
        float2 uvInterpolated = InterpolateBarycentrics(attr.barycentrics, uv);
    
        float rho = CalculateRayConeUVFootPrint(p, uv);
        float mipLevel = log2(rho);
        baseColor *= gTexture.SampleLevel(gSampler, uvInterpolated, mipLevel);
    }
    
    //float metallic = metallicFactor;
    //float roughness = roughnessFactor;
    
    //if (flags & HasMetallicRoughnessTex != 0)
    //{
    //    float4 mrSample = gTexture.SampleLevel(gSampler, uvInterpolated, mipLevel);
    //    metallic *= mrSample.b;
    //    roughness *= mrSample.g;
    //}

    

    float3 hitPosition    = HitWorldPosition();
    bool shadowHit = TraceShadowRay(hitPosition, lightDirection, payload.currentRecursionDepth);
    float shadowing = shadowHit ? 0.5f : 1.0f;
    payload.color = baseColor * shadowing;
}

[shader("anyhit")]
void AHSAlphaCutOff(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    uint3 indices = GetHitTriangleIndices();

    float3 p[3];
    float2 uv[3];
    
    GetValues(indices, posVbBuffer, p);
    GetValues(indices, uvVbBuffer, uv);
    
    float2 uvInterpolated = InterpolateBarycentrics(attr.barycentrics, uv);
    
    float rho = CalculateRayConeUVFootPrint(p, uv);
    float mipLevel = log2(rho);
    mipLevel = clamp(mipLevel, 0, 3); // Clamp mip level to valid range.)
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

[shader("closesthit")]
void CHSShadow(inout ShadowRayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    payload.isHit = true;
}

[shader("miss")]
void MyMissShader(inout RayPayload payload)
{
    payload.color = float4(0.1f, 0.1f, 0.4f, 1.0f);
}

[shader("miss")]
void ShadowMissShader(inout ShadowRayPayload payload)
{
    payload.isHit = false;
}