struct VSInput
{
    float3 position          : POSITION;
    float3 normal            : NORMAL;
};

struct PSInput
{
    float4 position          : SV_POSITION;
    float3 world_position    : POSITION0;
    float3 world_normal      : NORMAL;
    float3 albedo            : ALBEDO;
    float3 uvw               : UVFACE;
    float3 face_blend_weights: BLENDWEIGHT;
};

struct Constants
{
    float4   light_color;
    float    light_power;
    float    light_ambient_factor;
    float    light_specular_factor;
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
    float3   deep_color;
    float3   shallow_color;
    float2   depth_range;
};

ConstantBuffer<Constants>     g_constants        : register(b1);
ConstantBuffer<SceneUniforms> g_scene_uniforms   : register(b2);
ConstantBuffer<MeshUniforms>  g_mesh_uniforms    : register(b3);
Texture2DArray<float4>        g_face_textures    : register(t1);
SamplerState                  g_texture_sampler  : register(s1);

float linstep(float min, float max, float s)
{
    return saturate((s - min) / (max - min));
}

PSInput AsteroidVS(VSInput input)
{
    const float4 position = float4(input.position, 1.0f);
    const float  depth    = linstep(g_mesh_uniforms.depth_range.x, g_mesh_uniforms.depth_range.y, length(input.position.xyz));

    PSInput output;
    output.position          = mul(g_mesh_uniforms.mvp_matrix, position);
    output.world_position    = mul(g_mesh_uniforms.model_matrix, position).xyz;
    output.world_normal      = normalize(mul(g_mesh_uniforms.model_matrix, float4(input.normal, 0.0)).xyz);
    output.albedo            = lerp(g_mesh_uniforms.deep_color, g_mesh_uniforms.shallow_color, depth);

    // Prepare coordinates and blending weights for triplanar projection texturing
    output.uvw               = input.position / g_mesh_uniforms.depth_range.y * 0.5f + 0.5f;
    output.face_blend_weights = abs(normalize(input.position));
    output.face_blend_weights = saturate((output.face_blend_weights - 0.2f) * 7.0f);
    output.face_blend_weights /= (output.face_blend_weights.x + output.face_blend_weights.y + output.face_blend_weights.z).xxx;

    return output;
}

float4 AsteroidPS(PSInput input) : SV_TARGET
{
    const float3 fragment_to_light  = normalize(g_scene_uniforms.light_position - input.world_position);
    const float3 fragment_to_eye    = normalize(g_scene_uniforms.eye_position.xyz - input.world_position);
    const float3 light_reflected_from_fragment = reflect(-fragment_to_light, input.world_normal);

    // Triplanar projection sampling
    float3 texel_rgb = 0.0;
    texel_rgb += input.face_blend_weights.x * g_face_textures.Sample(g_texture_sampler, float3(input.uvw.yz, 0)).xyz;
    texel_rgb += input.face_blend_weights.y * g_face_textures.Sample(g_texture_sampler, float3(input.uvw.zx, 1)).xyz;
    texel_rgb += input.face_blend_weights.z * g_face_textures.Sample(g_texture_sampler, float3(input.uvw.xy, 2)).xyz;

    const float4 texel_color    = float4(texel_rgb * input.albedo, 1.0);
    const float4 ambient_color  = texel_color * g_constants.light_ambient_factor;
    const float4 base_color     = texel_color * g_constants.light_color * g_constants.light_power;

    const float  diffuse_part   = clamp(dot(fragment_to_light, input.world_normal), 0.0, 1.0);
    const float4 diffuse_color  = base_color * diffuse_part;

    const float  specular_part  = pow(clamp(dot(fragment_to_eye, light_reflected_from_fragment), 0.0, 1.0), g_constants.light_specular_factor);
    const float4 specular_color = base_color * specular_part;

    return ambient_color + diffuse_color + specular_color;
}
