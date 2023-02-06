/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy
Licensed under the Apache License, Version 2.0

*******************************************************************************

FILE: MethaneKit/Modules/Graphics/Extensions/Shaders/ScreenQuadConstants.h
Shader uniform structures shared between HLSL and C++ code via HLSL++

******************************************************************************/
#ifndef SCREEN_QUAD_CONSTANTS_H
#define SCREEN_QUAD_CONSTANTS_H

struct ScreenQuadConstants
{
    float4 blend_color;
};

#endif // SCREEN_QUAD_CONSTANTS_H
