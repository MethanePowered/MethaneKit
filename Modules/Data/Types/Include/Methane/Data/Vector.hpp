/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License"),
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Data/Vector.hpp
Template vector type alias to hlsl++ vectors of various types

******************************************************************************/

#include <hlsl++.h>

namespace Methane::Data
{

template<typename T, size_t vector_size, typename = std::enable_if_t<2 <= vector_size && vector_size <= 4>>
struct Vector
{
    using Type = void;
};

template<> struct Vector<int32_t, 2> { using Type = hlslpp::int2; };
template<> struct Vector<int32_t, 3> { using Type = hlslpp::int3; };
template<> struct Vector<int32_t, 4> { using Type = hlslpp::int4; };

template<> struct Vector<uint32_t, 2> { using Type = hlslpp::uint2; };
template<> struct Vector<uint32_t, 3> { using Type = hlslpp::uint3; };
template<> struct Vector<uint32_t, 4> { using Type = hlslpp::uint4; };

template<> struct Vector<float, 2> { using Type = hlslpp::float2; };
template<> struct Vector<float, 3> { using Type = hlslpp::float3; };
template<> struct Vector<float, 4> { using Type = hlslpp::float4; };

#ifdef HLSLPP_DOUBLE
template<> struct Vector<double, 2> { using Type = hlslpp::double2; };
template<> struct Vector<double, 3> { using Type = hlslpp::double3; };
template<> struct Vector<double, 4> { using Type = hlslpp::double4; };
#endif

} // namespace Methane::Data