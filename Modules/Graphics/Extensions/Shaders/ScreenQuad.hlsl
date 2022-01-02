/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: MethaneKit/Modules/Graphics/Extensions/Shaders/ScreenQuad.hlsl
Shaders for screen quad rendering with 2D texture

******************************************************************************/

// TEXTURE_DISABLED - disables texture sampling and draws just a colored quad

#ifndef TTEXEL
#define TTEXEL float4
#endif

#ifndef TPIXEL
#define TPIXEL float4
#endif

#ifndef VPIXEL
#define VPIXEL float4(1.f,1.f,1.f,1.f)
#endif

#ifndef RMASK
#define RMASK rgba
#endif

#ifndef WMASK
#define WMASK rgba
#endif

#include "ScreenQuadConstants.h"

struct VSInput
{
    float3 position : POSITION;
    float2 texcoord : TEXCOORD;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

ConstantBuffer<ScreenQuadConstants> g_constants : register(b1);

#ifndef TEXTURE_DISABLED
Texture2D<TTEXEL> g_texture : register(t0);
SamplerState      g_sampler : register(s0);
#endif

PSInput QuadVS(VSInput input)
{
    PSInput output;
    output.position = float4(input.position, 1.0f);
    output.texcoord = input.texcoord;
    return output;
}

TPIXEL QuadPS(PSInput input) : SV_TARGET
{
#ifdef TEXTURE_DISABLED
    return g_constants.blend_color;
#else
    TPIXEL color = VPIXEL;
    color.WMASK = g_texture.Sample(g_sampler, input.texcoord).RMASK;
    return color * g_constants.blend_color;
#endif
}
