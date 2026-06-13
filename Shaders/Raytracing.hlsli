struct RayPayload
{
    float4 color;
    uint   currentRecursionDepth;
};

struct ShadowRayPayload
{
    uint isHit;
    uint currentRecursionDepth;
};