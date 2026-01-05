struct VSInput
{
    float3 position : POSITION;
    float3 color    : COLOR;
    float2 texcoord : TEXCOORD;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 color    : COLOR0;
    float2 texcoord : TEXCOORD;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 color    : COLOR0;
    float2 texcoord : TEXCOORD;
};