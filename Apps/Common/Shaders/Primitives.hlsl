/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: MethaneKit/Apps/Common/Shaders/Primitives.hlsl
Common shader primitive functions.

Optional macro definitions:
  SRGB_GAMMA_PRECISION=[0|1|2]

******************************************************************************/

// SRGB gamma correction approximations
// http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html

#ifndef SRGB_GAMMA_PRECISION
#define SRGB_GAMMA_PRECISION 2
#endif

float3 ColorSrgbToLinear(float3 srgb_color)
{
#if SRGB_GAMMA_PRECISION == 0
    return pow(srgb_color, 2.233333333);
#else
    return srgb_color * (srgb_color * (srgb_color * 0.305306011 + 0.682171111) + 0.012522878);
#endif
}

float4 ColorSrgbToLinear(float4 srgb_color)
{
    return float4(ColorSrgbToLinear(srgb_color.rgb), srgb_color.a);
}

float3 ColorLinearToSrgb(float3 linear_color)
{
#if SRGB_GAMMA_PRECISION == 0
    return max(1.055 * pow(linear_color, 0.416666667) - 0.055, 0);
#else
    const float3 S1 = sqrt(linear_color);
    const float3 S2 = sqrt(S1);
    const float3 S3 = sqrt(S2);
#if SRGB_GAMMA_PRECISION == 1
    return 0.585122381 * S1 + 0.783140355 * S2 - 0.368262736 * S3;
#else
    return 0.662002687 * S1 + 0.684122060 * S2 - 0.323583601 * S3 - 0.0225411470 * linear_color;
#endif
#endif
}

float4 ColorLinearToSrgb(float4 linear_color)
{
    return float4(ColorLinearToSrgb(linear_color.rgb), linear_color.a);
}

float linstep(float min, float max, float s)
{
    return saturate((s - min) / (max - min));
}