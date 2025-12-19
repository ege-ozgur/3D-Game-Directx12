struct PS_IN
{
    float4 pos : SV_POSITION;
};

float4 PS(PS_IN input) : SV_TARGET
{
    return float4(1.0f, 0.0f, 0.0f, 1.0f); 
}

// simple pixel shader to render a red crosshair