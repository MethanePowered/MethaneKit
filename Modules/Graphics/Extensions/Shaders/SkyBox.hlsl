struct SkyboxUniforms
{
    float4x4 mvp_matrix;
};

ConstantBuffer<SkyboxUniforms> g_skybox_uniforms : register(b1);
TextureCube                    g_skybox_texture  : register(t1);
SamplerState                   g_texture_sampler : register(s1);

struct SkyboxVSOut
{
    float4 position : SV_POSITION;
    float3 uvw      : UVFACE;
};

SkyboxVSOut SkyboxVS(float3 in_position : POSITION)
{
    SkyboxVSOut output;

    output.position   = mul(g_skybox_uniforms.mvp_matrix, float4(in_position, 0.0f));
    output.position.z = 0.0f;
    output.uvw        = in_position;

    return output;
}

float4 SkyboxPS(SkyboxVSOut input) : SV_TARGET
{
    const float4 texel_color = g_skybox_texture.Sample(g_texture_sampler, input.uvw);
    return float4(texel_color.xyz, 1.f);
}