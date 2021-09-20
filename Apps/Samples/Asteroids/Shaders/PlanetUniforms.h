/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy
Licensed under the Apache License, Version 2.0

*******************************************************************************

FILE: MethaneKit/Apps/Samples/Asteroids/Shaders/PlanetUniforms.h
Shader uniform structures shared between HLSL and C++ code via HLSL++

******************************************************************************/
#ifndef PLANET_UNIFORMS_H
#define PLANET_UNIFORMS_H

#ifndef META_UNIFORM_ALIGN
#define META_UNIFORM_ALIGN
#endif

struct META_UNIFORM_ALIGN PlanetUniforms
{
    float4   eye_position;
    float3   light_position;
    float4x4 mvp_matrix;
    float4x4 model_matrix;
};

#endif // PLANET_UNIFORMS_H
