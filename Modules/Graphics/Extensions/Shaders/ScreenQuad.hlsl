struct VSInput
{
    float3 position         : POSITION;
    float2 texcoord         : TEXCOORD;
};

struct PSInput
{
    float4 position         : SV_POSITION;
    float2 texcoord         : TEXCOORD;
};

struct Constants
{
    float4 blend_color;
};

ConstantBuffer<Constants> g_constants : register(b1);
Texture2D                 g_texture   : register(t0);
SamplerState              g_sampler   : register(s0);

PSInput ScreenQuadVS(VSInput input)
{
    PSInput output;
    output.position = float4(input.position, 1.0f);
    output.texcoord = input.texcoord;
    return output;
}

float4 ScreenQuadPS(PSInput input) : SV_TARGET
{
    const float4 texel_color = g_texture.Sample(g_sampler, input.texcoord);
    return texel_color * g_constants.blend_color;
}
