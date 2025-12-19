cbuffer SkyboxConstantBuffer : register(b0)
{
    matrix WVP;
};

struct VS_INPUT
{
    float3 Pos : POSITION;
};

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float3 LocalPos : POSITION;
};

VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output;
    output.LocalPos = input.Pos;

    float4 pos = mul(float4(input.Pos, 1.0f), WVP);
    
    output.Pos = pos;
    output.Pos.z = pos.w * 0.999999f; // set depth to far plane

    return output;
}

// vertex shader for rendering a skybox