Texture2D albedoMap : register(t0);
SamplerState sam : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 PS(PSInput input) : SV_TARGET
{
    float2 tiledUV = input.uv * 6.0f;
    
    float4 color = albedoMap.Sample(sam, tiledUV);

    float3 N = float3(0.0f, 1.0f, 0.0f); // Upward normal for flat ground
    float3 lightDir = normalize(float3(0.5f, 1.5f, -0.5f)); // Directional light

    float diff = max(dot(N, lightDir), 0.0f); 
    float3 finalLight = diff + 0.5f; 

    return float4(color.rgb * finalLight, 1.0f); 
}

// pixel shader for rendering ground with simple directional lighting and tiled texture