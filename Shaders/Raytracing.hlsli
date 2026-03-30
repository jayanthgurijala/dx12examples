struct RayPayload
{
    float4 color;
    uint   currentRecursionDepth;
};

struct ShadowRayPayload
{
    bool isHit;
};