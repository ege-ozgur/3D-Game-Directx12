Texture2D tex : register(t0);
SamplerState sam : register(s0);

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float2 TexCoords : TEXCOORD;
};

float4 PS(PS_INPUT input) : SV_Target0
{
    float4 albedo = tex.Sample(sam, input.TexCoords);

    if (albedo.a < 0.5f) // we use this for the leaves on the trees
    {
        discard;
    }

    float3 lightDir = normalize(float3(0.5, 1.0, -0.5));
    float diff = max(dot(normalize(input.Normal), lightDir), 0.2f);

    return float4(albedo.rgb * diff, 1.0f);
}