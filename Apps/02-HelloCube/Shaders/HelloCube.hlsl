/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy

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

FILE: MethaneKit/Apps/Tutorials/02-HelloCube/Shaders/HelloCube.hlsl
Shaders for colored cube rendering

******************************************************************************/

#ifdef UNIFORMS_BUFFER_ENABLED
#include "HelloCubeUniforms.h"
#endif

struct VSInput
{
    float3 position : POSITION;
    float3 color    : COLOR;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
};

#ifdef UNIFORMS_BUFFER_ENABLED
ConstantBuffer<Uniforms> g_uniforms : register(b0);
#endif

PSInput CubeVS(VSInput input)
{
    PSInput output;
#ifdef UNIFORMS_BUFFER_ENABLED
    output.position = mul(float4(input.position, 1.F), g_uniforms.mvp_matrix);
#else
    output.position = float4(input.position, 1.F);
#endif
    output.color    = float4(input.color, 1.F);
    return output;
}

float4 CubePS(PSInput input) : SV_TARGET
{
    return input.color;
}
