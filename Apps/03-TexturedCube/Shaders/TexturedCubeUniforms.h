/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy
Licensed under the Apache License, Version 2.0

*******************************************************************************

FILE: MethaneKit/Apps/Tutorials/03-TexturedCube/Shaders/TexturedCubeUniforms.h
Shader uniform structures shared between HLSL and C++ code via HLSL++

******************************************************************************/
#ifndef TEXTURED_CUBE_UNIFORMS_H
#define TEXTURED_CUBE_UNIFORMS_H

struct Constants
{
    float4 light_color;
    float  light_power;
    float  light_ambient_factor;
    float  light_specular_factor;
    float  _padding;
};

struct Uniforms
{
    float3   eye_position;
    float3   light_position;
    float4x4 mvp_matrix;
    float4x4 model_matrix;
};

#endif // TEXTURED_CUBE_UNIFORMS_H
