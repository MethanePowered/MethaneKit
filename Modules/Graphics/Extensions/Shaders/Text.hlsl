/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: MethaneKit/Modules/Graphics/Extensions/Shaders/Text.hlsl
Shaders for text rendering using Glyphs atlas texture

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
    float4 blend_color;
};

ConstantBuffer<Constants> g_constants : register(b1);
Texture2D<float>          g_texture   : register(t0);
SamplerState              g_sampler   : register(s0);

PSInput TextVS(VSInput input)
{
    PSInput output;
    output.position = float4(input.position, 0.f, 1.0f);
    output.texcoord = input.texcoord;
    return output;
}

float4 TextPS(PSInput input) : SV_TARGET
{
    const float alpha = g_texture.Sample(g_sampler, input.texcoord);
    return float4(g_constants.blend_color.rgb, alpha);
}
