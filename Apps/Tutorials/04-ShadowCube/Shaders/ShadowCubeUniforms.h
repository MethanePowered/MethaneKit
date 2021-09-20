/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy
Licensed under the Apache License, Version 2.0

*******************************************************************************

FILE: MethaneKit/Apps/Tutorials/04-ShadowCube/Shaders/ShadowCubeUniforms.h
Shader uniform structures shared between HLSL and C++ code via HLSL++

******************************************************************************/
#ifndef SHADOW_CUBE_UNIFORMS_H
#define SHADOW_CUBE_UNIFORMS_H

struct Constants
{
    float4 light_color;
    float  light_power;
    float  light_ambient_factor;
    float  light_specular_factor;
};

struct SceneUniforms
{
    float4   eye_position;
    float3   light_position;
};

struct MeshUniforms
{
    float4x4 model_matrix;
    float4x4 mvp_matrix;
#ifdef ENABLE_SHADOWS
    float4x4 shadow_mvpx_matrix;
#endif
};

#endif // SHADOW_CUBE_UNIFORMS_H
