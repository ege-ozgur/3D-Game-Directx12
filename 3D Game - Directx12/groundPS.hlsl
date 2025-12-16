Texture2D albedoMap : register(t0);
SamplerState sam : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 uv : TEXCOORD;
};

float4 PS(PSInput input) : SV_TARGET
{
    float2 tiledUV = input.uv * 5.0f;
    float4 color = albedoMap.Sample(sam, tiledUV);
    float3 lightDir = normalize(float3(0.5f, 1.0f, -0.5f));
    float diff = max(dot(normalize(input.normal), lightDir), 0.0f);
    diff = pow(diff, 1.2f); 
    float3 finalLight = diff + 0.4f;
    return float4(color.rgb * finalLight, 1.0f);
}