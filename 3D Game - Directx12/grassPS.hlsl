Texture2D tex : register(t0);
SamplerState sam : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

float4 PS(PSInput input) : SV_TARGET
{
    float4 color = tex.Sample(sam, input.uv);

    if (color.a < 0.5f) // alpha testing threshold
        discard;

    return color;
}

// pixel shader for rendering grass with alpha testing