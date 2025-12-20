cbuffer SceneConstantBuffer : register(b0)
{
    matrix VP;
    float gTime; 
    float3 padding; 
};

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 uv : TEXCOORD;
    row_major matrix World : WORLD;
    uint instanceID : SV_InstanceID;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

PSInput VS(VSInput input)
{
    PSInput output;
    float heightFactor = input.position.y;
    float windStrength = 0.2f; // Rthe power of the wind
    float windSpeed = 2.0f; // the speed of the wind
    
    // formula for the wind
    float sway = sin(gTime * windSpeed + input.World[3][0]) * windStrength * heightFactor;

    input.position.x += sway;

    float4 worldPos = mul(float4(input.position, 1.0f), input.World);
    output.position = mul(worldPos, VP);
    output.uv = input.uv;
    output.normal = mul(input.normal, (float3x3) input.World);

    return output;
}