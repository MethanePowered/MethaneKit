/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Graphics/MathTypes.h
Math types aliases.

******************************************************************************/

#pragma once

#if defined _WIN32

#include "Windows/MathTypes.h"

#elif defined __APPLE__

#include "MacOS/MathTypes.h"

#elif defined __linux__

#include "Linux/MathTypes.h"

#endif

#include <cml/vector.h>
#include <cml/mathlib/constants.h>
#include <cml/matrix.h>

#include <string>

#define SHADER_STRUCT_ALIGNMENT 256
#define SHADER_STRUCT_ALIGN alignas(SHADER_STRUCT_ALIGNMENT)
#define SHADER_FIELD_ALIGN  alignas(16)
#define SHADER_FIELD_PACK   alignas(4)

namespace Methane::Graphics
{

constexpr cml::AxisOrientation g_axis_orientation = cml::AxisOrientation::left_handed;

// Use row-major order matrices for DirectX
using Matrix22i = cml::matrix22i_r;
using Matrix33i = cml::matrix33i_r;
using Matrix44i = cml::matrix44i_r;
using Matrix22f = cml::matrix22f_r;
using Matrix33f = cml::matrix33f_r;
using Matrix44f = cml::matrix44f_r;

using Vector2i = cml::vector2i;
using Vector3i = cml::vector3i;
using Vector4i = cml::vector4i;
using Vector2f = cml::vector2f;
using Vector3f = cml::vector3f;
using Vector4f = cml::vector4f;

template<typename T, int vector_size>
inline std::string VectorToString(const cml::vector<T, cml::fixed<vector_size>>& v)
{
    std::string str = "V(";
    for (int i = 0; i < vector_size; ++i)
    {
        str += std::to_string(v[i]);
        if (i < vector_size - 1)
            str += ", ";
    }
    return str + ")";
}

}
