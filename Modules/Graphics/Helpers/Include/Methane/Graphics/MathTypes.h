/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

#include <cml/vector.h>
#include <cml/quaternion.h>
#include <string>

#if defined _WIN32

#include "Windows/MathTypes.h"

#elif defined __APPLE__

#include "MacOS/MathTypes.h"

#endif

namespace Methane
{
namespace Graphics
{

using Vector2i = cml::vector2i;
using Vector3i = cml::vector3i;
using Vector4i = cml::vector4i;
using Vector2f = cml::vector2f;
using Vector3f = cml::vector3f;
using Vector4f = cml::vector4f;

using Quaternionf = cml::quaternionf;

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
}
