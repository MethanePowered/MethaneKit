struct Constants
{
    float4 light_color;
    float  light_power;
    float  light_ambient_factor;
    float  light_specular_factor;
};

struct SceneUniforms
{
    float4   eye_position;
    float3   light_position;
};

struct MeshUniforms
{
    float4x4 model_matrix;
    float4x4 mvp_matrix;
};

ConstantBuffer<Constants>     g_constants           : register(b1);
ConstantBuffer<SceneUniforms> g_scene_uniforms      : register(b2);
ConstantBuffer<MeshUniforms>  g_mesh_uniforms       : register(b3);
Texture2D                     g_texture             : register(t1);
SamplerState                  g_texture_sampler     : register(s1);

struct PSInput
{
    float4 position             : SV_POSITION;
    float3 world_position       : POSITION0;
    float3 normal               : NORMAL;
    float2 uv                   : TEXCOORD;
};

struct VSPositionOut
{
    float4 position : SV_POSITION;
};

VSPositionOut VSPositionOnly(float3 in_position : POSITION)
{
    VSPositionOut output;
    output.position = mul(g_mesh_uniforms.mvp_matrix, float4(in_position, 1.0f));
    return output;
}

PSInput AsteroidsVS(float3 in_position : POSITION, 
               float3 in_normal   : NORMAL,
               float2 in_uv       : TEXCOORD)
{
    const float4 position       = float4(in_position, 1.0f);
    const float3 normal         = normalize(mul(float4(in_normal, 0.0), g_mesh_uniforms.model_matrix)).xyz;

    PSInput output;
    output.position             = mul(g_mesh_uniforms.mvp_matrix, position);
    output.world_position       = mul(g_mesh_uniforms.model_matrix, position).xyz;
    output.normal               = normalize(mul(g_mesh_uniforms.model_matrix, float4(normal, 0.0)).xyz);
    output.uv                   = in_uv;

    return output;
}

float4 AsteroidsPS(PSInput input) : SV_TARGET
{
    const float3 fragment_to_light  = normalize(g_scene_uniforms.light_position - input.world_position);
    const float3 fragment_to_eye    = normalize(g_scene_uniforms.eye_position.xyz - input.world_position);
    const float3 light_reflected_from_fragment = reflect(-fragment_to_light, input.normal);

    const float4 texel_color    = g_texture.Sample(g_texture_sampler, input.uv);
    const float4 ambient_color  = texel_color * g_constants.light_ambient_factor;
    const float4 base_color     = texel_color * g_constants.light_color * g_constants.light_power;

    const float distance        = length(g_scene_uniforms.light_position - input.world_position);
    const float diffuse_part    = clamp(dot(fragment_to_light, input.normal), 0.0, 1.0);
    const float4 diffuse_color  = base_color * diffuse_part / (distance * distance);

    const float specular_part   = pow(clamp(dot(fragment_to_eye, light_reflected_from_fragment), 0.0, 1.0), g_constants.light_specular_factor);
    const float4 specular_color = base_color * specular_part / (distance * distance);

    return ambient_color + diffuse_color + specular_color;
}
