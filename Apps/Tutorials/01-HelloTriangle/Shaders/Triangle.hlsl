struct PSInput
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
};

PSInput TriangleVS(float3 in_position : POSITION, 
               float3 in_color    : COLOR)
{
    PSInput output;
    output.position = float4(in_position, 1.0f);
    output.color    = float4(in_color, 1.0f);
    return output;
}

float4 TrianglePS(PSInput input) : SV_TARGET
{
    return input.color;
}
