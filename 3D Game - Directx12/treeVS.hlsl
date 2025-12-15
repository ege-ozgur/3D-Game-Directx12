cbuffer SceneConstantBuffer : register(b0)
{
    float4x4 VP;
};

struct VS_INPUT
{
    float4 Pos : POSITION;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float2 TexCoords : TEXCOORD;
    // Agaclarin patlamamasi icin row_major sart
    row_major float4x4 World : WORLD;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 Normal : NORMAL;
    float2 TexCoords : TEXCOORD;
};

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output;
    float4 worldPos = mul(input.Pos, input.World);
    output.Pos = mul(worldPos, VP);
    output.Normal = normalize(mul(input.Normal, (float3x3) input.World));
    output.TexCoords = input.TexCoords;
    return output;
}