cbuffer ConstantBuffer : register(b0)
{
    matrix W;
    matrix VP;
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
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 uv : TEXCOORD;
};

PSInput VS(VSInput input)
{
    PSInput output;
    float4 wPos = mul(float4(input.position, 1.0f), W);
    output.worldPos = wPos.xyz;
    output.position = mul(wPos, VP);
    output.normal = normalize(mul(input.normal, (float3x3) W));
    output.tangent = normalize(mul(input.tangent, (float3x3) W));
    output.uv = input.uv;
    return output;
}