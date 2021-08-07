/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy
Licensed under the Apache License, Version 2.0

*******************************************************************************

FILE: MethaneKit/Apps/Samples/Asteroids/Shaders/SceneConstants.h
Shader uniform structures shared between HLSL and C++ code via HLSL++

******************************************************************************/
#ifndef SCENE_CONSTANTS_H
#define SCENE_CONSTANTS_H

#ifndef META_UNIFORM_ALIGN
#define META_UNIFORM_ALIGN
#endif

struct META_UNIFORM_ALIGN SceneConstants
{
    float4 light_color;
    float  light_power;
    float  light_ambient_factor;
    float  light_specular_factor;
};

#endif // SCENE_CONSTANTS_H
