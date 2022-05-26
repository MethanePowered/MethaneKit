/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy
Licensed under the Apache License, Version 2.0

*******************************************************************************

FILE: MethaneKit/Apps/Tutorials/06-CubeMapArray/Shaders/CubeMapArrayUniforms.h
Shader uniform structures shared between HLSL and C++ code via HLSL++

******************************************************************************/
#ifndef CUBE_MAP_ARRAY_UNIFORMS_H
#define CUBE_MAP_ARRAY_UNIFORMS_H

#define CUBE_MAP_ARRAY_SIZE 8

struct META_UNIFORM_ALIGN Uniforms
{
    float4x4 mvp_matrix_per_instance[CUBE_MAP_ARRAY_SIZE];
};

#endif // CUBE_MAP_ARRAY_UNIFORMS_H
