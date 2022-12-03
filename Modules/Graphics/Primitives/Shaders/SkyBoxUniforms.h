/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy
Licensed under the Apache License, Version 2.0

*******************************************************************************

FILE: MethaneKit/Modules/Graphics/Extensions/Shaders/SkyBoxUniforms.h
Shader uniform structures shared between HLSL and C++ code via HLSL++

******************************************************************************/
#ifndef SKY_BOX_UNIFORMS_H
#define SKY_BOX_UNIFORMS_H

struct SkyBoxUniforms
{
    float4x4 mvp_matrix;
};

#endif // SKY_BOX_UNIFORMS_H
