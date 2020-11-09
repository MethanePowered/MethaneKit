/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: MethaneKit/Modules/UserInterface/Typography/Shaders/Text.hlsl
Shaders for text rendering from glyphs atlas texture

******************************************************************************/

struct VSInput
{
    float2 position         : POSITION;
    float2 texcoord         : TEXCOORD;
};

struct PSInput
{
    float4 position         : SV_POSITION;
    float2 texcoord         : TEXCOORD;
};

struct Constants
{
    float4 color;
};

struct Uniforms
{
    float4x4 vp_matrix;
};

ConstantBuffer<Constants> g_constants : register(b1);
ConstantBuffer<Uniforms>  g_uniforms  : register(b2);
Texture2D<float>          g_texture   : register(t0);
SamplerState              g_sampler   : register(s0);

PSInput TextVS(VSInput input)
{
    PSInput output;
    output.position = float4(mul(g_uniforms.vp_matrix, float4(input.position, 1.F, 1.F)).xy, 0.F, 1.F);
    output.texcoord = input.texcoord;
    return output;
}

float4 TextPS(PSInput input) : SV_TARGET
{
    const float glyph_alpha = g_texture.Sample(g_sampler, input.texcoord);
    return float4(g_constants.color.rgb, g_constants.color.a * glyph_alpha);
}
