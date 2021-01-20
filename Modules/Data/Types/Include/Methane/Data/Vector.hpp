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
Template vector type alias for HLSL++ vectors of various types and sizes

******************************************************************************/

#pragma once

#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>

#include <hlsl++.h>

namespace Methane::Data
{

template<typename T, size_t size, typename = std::enable_if_t<std::is_arithmetic_v<T> && 2 <= size && size <= 4>>
struct HlslVectorMap
{
    using Type = void;
};

template<> struct HlslVectorMap<int32_t, 2> { using Type = hlslpp::int2; };
template<> struct HlslVectorMap<int32_t, 3> { using Type = hlslpp::int3; };
template<> struct HlslVectorMap<int32_t, 4> { using Type = hlslpp::int4; };

template<> struct HlslVectorMap<uint32_t, 2> { using Type = hlslpp::uint2; };
template<> struct HlslVectorMap<uint32_t, 3> { using Type = hlslpp::uint3; };
template<> struct HlslVectorMap<uint32_t, 4> { using Type = hlslpp::uint4; };

template<> struct HlslVectorMap<float, 2> { using Type = hlslpp::float2; };
template<> struct HlslVectorMap<float, 3> { using Type = hlslpp::float3; };
template<> struct HlslVectorMap<float, 4> { using Type = hlslpp::float4; };

#ifdef HLSLPP_DOUBLE
template<> struct HlslVectorMap<double, 2> { using Type = hlslpp::double2; };
template<> struct HlslVectorMap<double, 3> { using Type = hlslpp::double3; };
template<> struct HlslVectorMap<double, 4> { using Type = hlslpp::double4; };
#endif

template<typename T, size_t size>
using HlslVector = typename HlslVectorMap<T, size>::Type;

template<typename T, size_t size>
T GetVectorComponent(const HlslVector<T, size>& vec, size_t index)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS(index, size);
    switch(index)
    {
    case 0: return vec.x;
    case 1: return vec.y;
    case 2: if constexpr (size >= 3) return vec.z; break;
    case 3: if constexpr (size == 4) return vec.w; break;
    default: META_UNEXPECTED_ARG(index);
    }
}

template<typename T, size_t size, typename = std::enable_if_t<std::is_arithmetic_v<T> && 2 <= size && size <= 4>>
class RawVector
{
public:
    static constexpr size_t Size = size;

    RawVector() = default;

    template<typename ...TArgs>
    RawVector(TArgs... args) noexcept : m_components({ args... }) { }

    template<size_t sz = size, typename = std::enable_if_t<sz == 2>>
    explicit RawVector(const HlslVector<T, 2>& vec) noexcept: m_components({ vec.x, vec.y }) { }

    template<size_t sz = size, typename = std::enable_if_t<sz == 3>>
    explicit RawVector(const HlslVector<T, 3>& vec) noexcept : m_components({ vec.x, vec.y, vec.z }) { }

    template<size_t sz = size, typename = std::enable_if_t<sz == 4>>
    explicit RawVector(const HlslVector<T, 4>& vec) noexcept : m_components({ vec.x, vec.y, vec.z, vec.w }) { }

    template<size_t sz = size, typename = std::enable_if_t<sz == 2>>
    operator HlslVector<T, 2>() const noexcept { return HlslVector<T, 2>(GetX(), GetY()); }

    template<size_t sz = size, typename = std::enable_if_t<sz == 3>>
    operator HlslVector<T, 3>() const noexcept { return HlslVector<T, 3>(GetX(), GetY(), GetZ()); }

    template<size_t sz = size, typename = std::enable_if_t<sz == 4>>
    operator HlslVector<T, 4>() const noexcept { return HlslVector<T, 4>(GetX(), GetY(), GetZ(), GetW()); }

    T operator[](size_t index) const noexcept { return m_components[index]; }
    T Get(size_t index) const { META_CHECK_ARG_LESS(index, size); return m_components[index]; }

    T GetX() const noexcept { return m_components[0]; }
    T GetY() const noexcept { return m_components[1]; }

    template<size_t sz = size, typename = std::enable_if_t<sz >= 3>>
    T GetZ() const noexcept { return m_components[2]; }

    template<size_t sz = size, typename = std::enable_if_t<sz == 4>>
    T GetW() const noexcept { return m_components[3]; }

    void SetX(T x) noexcept { m_components[0] = x; }
    void SetY(T y) noexcept { m_components[1] = y; }

    template<size_t sz = size, typename = std::enable_if_t<sz >= 3>>
    void SetZ(T z) noexcept { return m_components[2] = z; }

    template<size_t sz = size, typename = std::enable_if_t<sz >= 4>>
    void SetW(T w) noexcept { return m_components[3] = w; }

private:
    T m_components[size]{ };
};

} // namespace Methane::Data