struct VS_IN
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 tex : TEXCOORD;
};

struct PS_IN
{
    float4 pos : SV_POSITION;
};

PS_IN VS(VS_IN input)
{
    PS_IN output;
    output.pos = float4(input.pos, 1.0f);
    return output;
}

// simple vertex shader to render a crosshair at the center of the screen