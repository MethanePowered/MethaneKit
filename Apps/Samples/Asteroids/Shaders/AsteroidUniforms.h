/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy
Licensed under the Apache License, Version 2.0

*******************************************************************************

FILE: MethaneKit/Apps/Samples/Asteroids/Shaders/AsteroidUniforms.h
Shader uniform structures shared between HLSL and C++ code via HLSL++

******************************************************************************/
#ifndef ASTEROID_UNIFORMS_H
#define ASTEROID_UNIFORMS_H

#ifdef __cplusplus
using uint = uint32_t;
#endif

#ifndef META_UNIFORM_ALIGN
#define META_UNIFORM_ALIGN
#endif

struct META_UNIFORM_ALIGN SceneUniforms
{
    float4x4 view_proj_matrix;
    float3   eye_position;
    float3   light_position;
};

struct META_UNIFORM_ALIGN AsteroidUniforms
{
    float4x4 model_matrix;
    float3   deep_color;
    float3   shallow_color;
#ifndef __cplusplus
    float    _;
#endif
    float    depth_min;
    float    depth_max;
    uint     texture_index;
};

#endif // ASTEROID_UNIFORMS_H
