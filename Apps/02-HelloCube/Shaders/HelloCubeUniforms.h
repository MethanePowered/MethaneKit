/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy
Licensed under the Apache License, Version 2.0

*******************************************************************************

FILE: MethaneKit/Apps/Tutorials/02-HelloCubeUniforms/Shaders/HelloCubeUniforms.h
Shader uniform structure shared between HLSL and C++ code via HLSL++

******************************************************************************/
#ifndef HELLO_CUBE_UNIFORMS_H
#define HELLO_CUBE_UNIFORMS_H

struct Uniforms
{
    float4x4 mvp_matrix;
};

#endif // HELLO_CUBE_UNIFORMS_H
