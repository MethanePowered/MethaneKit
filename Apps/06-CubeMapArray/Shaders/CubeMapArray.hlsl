/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: MethaneKit/Apps/Tutorials/06-CubeMapArray/Shaders/CubeMapArray.hlsl
Shaders for cube-map array textured rendering with Phong lighting model

******************************************************************************/

#include "CubeMapArrayUniforms.h"
#include "..\..\Common\Shaders\Primitives.hlsl"

struct VSInput
{
    uint   instance_id : SV_InstanceID;
    float3 position    : POSITION;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 uvwi      : UVFACE;
};

ConstantBuffer<Uniforms>  g_uniforms      : register(b0, META_ARG_FRAME_CONSTANT);
TextureCubeArray          g_texture_array : register(t0, META_ARG_CONSTANT);
SamplerState              g_sampler       : register(s0, META_ARG_CONSTANT);

PSInput CubeVS(VSInput input)
{
    PSInput output;
    output.position = mul(float4(input.position, 1.F), g_uniforms.mvp_matrix_per_instance[input.instance_id]);
    output.uvwi     = float4(-input.position, input.instance_id); // use position with negative sign to fix texture reflection
    return output;
}

float4 CubePS(PSInput input) : SV_TARGET
{
    return g_texture_array.Sample(g_sampler, input.uvwi);
}
