struct VSInput_1
{
    float3 position : POSITION;
};

struct VSOutput_1
{
    float4 position : SV_POSITION;
};

struct PSInput_1
{
    float4 position : SV_POSITION;
};



//<------------------------------ FOR 5 input attributes ----------------->
struct VSInputPrimitive
{
    float3 position  : POSITION;
    float3 normal    : NORMAL;
    float2 texcoord0 : TEXCOORD0;
    float2 texcoord1 : TEXCOORD1;
    float2 texcoord2 : TEXCOORD2;
};

struct VSOutputPrimitive
{
    float4 position  : SV_POSITION;
    float2 texcoord0 : TEXCOORD0;
};

struct PSInputPrimitive
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

//<-------------------------------------END -------------------------------->


//<------------------- Frame Composition ----------------------------->

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