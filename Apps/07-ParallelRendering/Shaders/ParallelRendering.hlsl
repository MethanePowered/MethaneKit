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

FILE: MethaneKit/Apps/Tutorials/07-ParallelRendering/Shaders/ParallelRendering.hlsl
Shaders for cube rendering with sampling from Texture2DArray in parallel rendering tutorial

******************************************************************************/

#include "ParallelRenderingUniforms.h"

struct VSInput
{
    float3 position    : POSITION;
    float2 texcoord    : TEXCOORD;
};

struct PSInput
{
    float4 position    : SV_POSITION;
    float2 texcoord    : TEXCOORD;
};

ConstantBuffer<Uniforms>  g_uniforms      : register(b0, META_ARG_MUTABLE);
Texture2DArray            g_texture_array : register(t0, META_ARG_CONSTANT);
SamplerState              g_sampler       : register(s0, META_ARG_CONSTANT);

PSInput CubeVS(VSInput input)
{
    PSInput output;
    output.position = mul(float4(input.position, 1.F), g_uniforms.mvp_matrix);
    output.texcoord = input.texcoord;
    return output;
}

float4 CubePS(PSInput input) : SV_TARGET
{
    return g_texture_array.Sample(g_sampler, float3(input.texcoord, g_uniforms.texture_index));
}
