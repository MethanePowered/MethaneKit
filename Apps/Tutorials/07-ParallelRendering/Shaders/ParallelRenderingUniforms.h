/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy
Licensed under the Apache License, Version 2.0

*******************************************************************************

FILE: MethaneKit/Apps/Tutorials/07-ParallelRendering/Shaders/ParallelRenderingUniforms.h
Shader uniform structures shared between HLSL and C++ code via HLSL++

******************************************************************************/
#ifndef CUBE_MAP_ARRAY_UNIFORMS_H
#define CUBE_MAP_ARRAY_UNIFORMS_H

#ifdef __cplusplus
using uint = uint32_t;
#endif

#ifndef META_UNIFORM_ALIGN
#define META_UNIFORM_ALIGN
#endif

struct META_UNIFORM_ALIGN Uniforms
{
    float4x4 mvp_matrix;
    int      texture_index;
};

#endif // CUBE_MAP_ARRAY_UNIFORMS_H
