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



//<------------------------------ FOR 2 input attributes ----------------->

struct VSInput_2
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct VSOutput_2
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    
};

struct PSInput_2
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
};

//<------------------------------ FOR 3 input attributes ----------------->

struct VSInput_3
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD0;
};

struct VSOutput_3
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD0;
};

struct PSInput_3
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD0;
};

//<------------------------------ FOR 5 input attributes ----------------->
struct VSInput_5
{
    float3 position  : POSITION;
    float3 normal    : NORMAL;
    float2 texcoord0 : TEXCOORD0;
    float2 texcoord1 : TEXCOORD1;
    float2 texcoord2 : TEXCOORD2;
};

struct VSOutput_5
{
    float4 position  : SV_POSITION;
    float4 worldPosition : WORLDPOSITION;
    float3 normal    : NORMAL;
    float2 texcoord0 : TEXCOORD0;
    float2 texcoord1 : TEXCOORD1;
    float2 texcoord2 : TEXCOORD2;
};

struct PSInput_5
{
    float4 position : SV_POSITION;
    float4 worldPosition : WORLDPOSITION;
    float3 normal   : NORMAL;
    float2 texcoord0 : TEXCOORD0;
    float2 texcoord1 : TEXCOORD1;
    float2 texcoord2 : TEXCOORD2;
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


//<---------------------------- Simple Tess -------------------------->

struct VSOutput_5_Tess
{
    float4 position  : SV_POSITION;
    float3 normal    : NORMAL;
    float2 texcoord0 : TEXCOORD0;
    float2 texcoord1 : TEXCOORD1;
    float2 texcoord2 : TEXCOORD2;
};

struct HSOutput_5
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texcoord0 : TEXCOORD0;
    float2 texcoord1 : TEXCOORD1;
    float2 texcoord2 : TEXCOORD2;
};

struct HSConstantsTriOutput
{
    float TessLevelOuter[3] : SV_TessFactor;
    float TessLevelInner : SV_InsideTessFactor;
};


//@note same as VSOUTPUT for non tess
struct DSOutput_5
{
    float4 position : SV_POSITION;
    float4 worldPosition : WORLDPOSITION;
    float3 normal : NORMAL;
    float2 texcoord0 : TEXCOORD0;
    float2 texcoord1 : TEXCOORD1;
    float2 texcoord2 : TEXCOORD2;
};




