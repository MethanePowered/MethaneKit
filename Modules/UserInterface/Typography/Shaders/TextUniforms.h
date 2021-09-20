/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy
Licensed under the Apache License, Version 2.0

*******************************************************************************

FILE: MethaneKit/Modules/UserInterface/Typography/Shaders/TextUniforms.h
Shader uniform structures shared between HLSL and C++ code via HLSL++

******************************************************************************/
#ifndef TEXT_UNIFORMS_H
#define TEXT_UNIFORMS_H

struct TextConstants
{
    float4 color;
};

struct TextUniforms
{
    float4x4 vp_matrix;
};

#endif // TEXT_UNIFORMS_H
