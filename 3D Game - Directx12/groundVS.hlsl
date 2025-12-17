cbuffer ConstantBuffer : register(b0)
{
    float4x4 W;
    float4x4 VP;
};

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 uv : TEXCOORD;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

PSInput VS(VSInput input)
{
    PSInput output;

    float4 wPos = mul(float4(input.position, 1.0f), W);
    output.position = mul(wPos, VP);
    output.uv = input.uv;

    return output;
}