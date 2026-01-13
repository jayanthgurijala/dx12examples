struct VSInputScene
{
    float3 position  : POSITION;
    float3 color     : COLOR;
    float2 texcoord0 : TEXCOORD_0;
    float2 texcoord1 : TEXCOORD_1;
    float2 texcoord2 : TEXCOORD_2;
};

struct VSOutputScene
{
    float4 position  : SV_POSITION;
    float3 color     : COLOR0;
    float2 texcoord0 : TEXCOORD_0;
};

struct PSInputScene
{
    float4 position : SV_POSITION;
    float3 color    : COLOR0;
    float2 texcoord : TEXCOORD_0;
};



struct VSInputSimpleFrameComposition
{
    float3 position : POSITION;
    float2 texcoord : TEXCOORD;
};

struct VSOutputSimpleFrameComposition
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

struct PSInputSimpleFrameComposition
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD;
};