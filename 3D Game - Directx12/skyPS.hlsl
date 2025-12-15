Texture2D gEquirectangularMap : register(t0);
SamplerState gSampler : register(s0);

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 LocalPos : POSITION;
};

static const float2 invAtan = float2(0.1591, 0.3183);

float4 PS(PS_INPUT input) : SV_Target
{
    float3 v = normalize(input.LocalPos);
    float2 uv = float2(atan2(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return gEquirectangularMap.Sample(gSampler, uv);
}