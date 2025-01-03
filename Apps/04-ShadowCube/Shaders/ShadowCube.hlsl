/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License"),
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: MethaneKit/Apps/Tutorials/04-ShadowCube/Shaders/ShadowCube.hlsl
Shaders for shadow and final pass rendering of textured cube
with Phong lighting model and simple shadows

Optional macro definition: ENABLE_SHADOWS, ENABLE_TEXTURING

******************************************************************************/

#include "ShadowCubeUniforms.h"
#include "../../Common/Shaders/Primitives.hlsl"

struct VSInput
{
    float3 position         : POSITION;
    float3 normal           : NORMAL;
#ifdef ENABLE_TEXTURING
    float2 texcoord         : TEXCOORD;
#endif
};

struct PSInput
{
    float4 position         : SV_POSITION;
    float3 world_position   : POSITION0;
    float3 world_normal     : NORMAL;
#ifdef ENABLE_SHADOWS
    float4 shadow_position  : POSITION1;
#endif
#ifdef ENABLE_TEXTURING
    float2 texcoord         : TEXCOORD;
#endif
};

ConstantBuffer<Constants>     g_constants       : register(b0, META_ARG_CONSTANT);
ConstantBuffer<SceneUniforms> g_scene_uniforms  : register(b1, META_ARG_FRAME_CONSTANT);
ConstantBuffer<MeshUniforms>  g_mesh_uniforms   : register(b2, META_ARG_MUTABLE);

#ifdef ENABLE_SHADOWS
Texture2D    g_shadow_map      : register(t0, META_ARG_FRAME_CONSTANT);
SamplerState g_shadow_sampler  : register(s0, META_ARG_CONSTANT);
#endif

#ifdef ENABLE_TEXTURING
Texture2D    g_texture         : register(t1, META_ARG_MUTABLE);
SamplerState g_texture_sampler : register(s1, META_ARG_CONSTANT);
#endif

PSInput CubeVS(VSInput input)
{
    const float4 position   = float4(input.position, 1.0F);

    PSInput output;
    output.position         = mul(position, g_mesh_uniforms.mvp_matrix);
    output.world_position   = mul(position, g_mesh_uniforms.model_matrix).xyz;
    output.world_normal     = normalize(mul(float4(input.normal, 0.0), g_mesh_uniforms.model_matrix).xyz);
#ifdef ENABLE_SHADOWS
    output.shadow_position  = mul(position, g_mesh_uniforms.shadow_mvpx_matrix);
#endif
#ifdef ENABLE_TEXTURING
    output.texcoord         = input.texcoord;
#endif

    return output;
}

float4 CubePS(PSInput input) : SV_TARGET
{
    const float3 fragment_to_light             = normalize(g_scene_uniforms.light_position.xyz - input.world_position);
    const float3 fragment_to_eye               = normalize(g_scene_uniforms.eye_position.xyz - input.world_position);
    const float3 light_reflected_from_fragment = reflect(-fragment_to_light, input.world_normal);

#ifdef ENABLE_SHADOWS
    const float3 light_proj_pos = input.shadow_position.xyz / input.shadow_position.w;
    const float  current_depth  = light_proj_pos.z - 0.0001F;
    const float  shadow_depth   = g_shadow_map.Sample(g_shadow_sampler, light_proj_pos.xy).r;
    const float  shadow_ratio   = current_depth > shadow_depth ? 1.0F : 0.0F;
#else
    const float  shadow_ratio   = 0.F;
#endif

#ifdef ENABLE_TEXTURING
    const float4 texel_color    = g_texture.Sample(g_texture_sampler, input.texcoord);
#else
    const float4 texel_color    = { 0.8F, 0.8F, 0.8F, 1.F };
#endif

    const float4 ambient_color  = texel_color * g_constants.light_ambient_factor;
    const float4 base_color     = texel_color * g_constants.light_color * g_constants.light_power;

    const float  distance       = length(g_scene_uniforms.light_position.xyz - input.world_position);
    const float  diffuse_part   = clamp(dot(fragment_to_light, input.world_normal), 0.0, 1.0);
    const float4 diffuse_color  = base_color * diffuse_part / (distance * distance);

    const float  specular_part  = pow(clamp(dot(fragment_to_eye, light_reflected_from_fragment), 0.0, 1.0), g_constants.light_specular_factor);
    const float4 specular_color = base_color * specular_part / (distance * distance);

    return ColorLinearToSrgb(ambient_color + (1.F - shadow_ratio) * (diffuse_color + specular_color));
}
