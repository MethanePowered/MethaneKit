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

FILE: Methane/Graphics/MacOS/MathTypes.h
MacOS specific math types aliases.

******************************************************************************/

#pragma once

#include <cml/mathlib/constants.h>
#include <cml/matrix.h>

#define SHADER_STRUCT_ALIGN alignas(256)
#define SHADER_FIELD_ALIGN  alignas(16)
#define SHADER_FIELD_PACK   alignas(4)

namespace Methane::Graphics
{

// Use row-major order matrices for Metal
using Matrix22i = cml::matrix22i_r;
using Matrix33i = cml::matrix33i_r;
using Matrix44i = cml::matrix44i_r;
using Matrix22f = cml::matrix22f_r;
using Matrix33f = cml::matrix33f_r;
using Matrix44f = cml::matrix44f_r;

// Axis orientation used by Metal
constexpr cml::AxisOrientation g_axis_orientation = cml::AxisOrientation::left_handed;

}
