Texture2D tex : register(t0);
SamplerState smp : register(s0);

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 Normal : NORMAL;
    float2 TexCoords : TEXCOORD;
};

float4 PS(PS_INPUT input) : SV_Target
{
    float4 color = tex.Sample(smp, input.TexCoords);
    
    if (color.a < 0.5f)
        discard;
    
    return color;
}