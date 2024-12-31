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
Template vector type for arithmetic scalar type and fixed size:
 - HlslVector<T, size> - alias to HLSL++ vector based on 128-bit XMM aligned memory
                         sizeof(HlslVector<T, size>) == 16 independent from T and size
 - RawVector<T, size>  - wrapper type around raw array T[size] for dense data packing
                         sizeof(RawVector<T, size>) == sizeof(T) * size
                         it can be created from or casted to HlslVector<T, size>

******************************************************************************/

#pragma once

#include "Math.hpp"

#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>

#include <hlsl++_vector_int.h>
#include <hlsl++_vector_uint.h>
#include <hlsl++_vector_float.h>
#include <hlsl++_vector_double.h>
#include <hlsl++_dependent.h>
#include <fmt/format.h>
#include <string>

namespace Methane::Data
{

namespace Internal
{

template<typename T, size_t size, typename = std::enable_if_t<std::is_arithmetic_v<T> && 1 <= size && size <= 4>>
struct HlslVectorMap
{
    using Type = void;
};

template<> struct HlslVectorMap<int32_t, 1> { using Type = hlslpp::int1; };
template<> struct HlslVectorMap<int32_t, 2> { using Type = hlslpp::int2; };
template<> struct HlslVectorMap<int32_t, 3> { using Type = hlslpp::int3; };
template<> struct HlslVectorMap<int32_t, 4> { using Type = hlslpp::int4; };

template<> struct HlslVectorMap<uint32_t, 1> { using Type = hlslpp::uint1; };
template<> struct HlslVectorMap<uint32_t, 2> { using Type = hlslpp::uint2; };
template<> struct HlslVectorMap<uint32_t, 3> { using Type = hlslpp::uint3; };
template<> struct HlslVectorMap<uint32_t, 4> { using Type = hlslpp::uint4; };

template<> struct HlslVectorMap<float, 1> { using Type = hlslpp::float1; };
template<> struct HlslVectorMap<float, 2> { using Type = hlslpp::float2; };
template<> struct HlslVectorMap<float, 3> { using Type = hlslpp::float3; };
template<> struct HlslVectorMap<float, 4> { using Type = hlslpp::float4; };

#ifdef HLSLPP_DOUBLE
template<> struct HlslVectorMap<double, 1> { using Type = hlslpp::double1; };
template<> struct HlslVectorMap<double, 2> { using Type = hlslpp::double2; };
template<> struct HlslVectorMap<double, 3> { using Type = hlslpp::double3; };
template<> struct HlslVectorMap<double, 4> { using Type = hlslpp::double4; };
#endif

} // namespace Internal

template<typename T, size_t size, typename = std::enable_if_t<std::is_arithmetic_v<T> && 1 <= size && size <= 4>>
using HlslVector = typename Internal::HlslVectorMap<T, size>::Type;

template<typename T, size_t size, typename = std::enable_if_t<std::is_arithmetic_v<T> && 1 <= size && size <= 4>>
T GetHlslVectorComponent(const HlslVector<T, size>& vec, size_t index)
{
    META_FUNCTION_TASK();
    META_CHECK_LESS(index, size);
    switch(index)
    {
    case 0: return vec.x;
    case 1: return vec.y;
    case 2: if constexpr (size >= 3) return vec.z; break;
    case 3: if constexpr (size == 4) return vec.w; break;
    default: META_UNEXPECTED(index);
    }
}

template<typename T, size_t size, typename = std::enable_if_t<std::is_arithmetic_v<T> && 2 <= size && size <= 4>>
HlslVector<T, size> CreateHlslVector(const std::array<T, size>& components) noexcept
{
    META_FUNCTION_TASK();
    if constexpr (size == 2)
        return HlslVector<T, 2>(components[0], components[1]);
    if constexpr (size == 3)
        return HlslVector<T, 3>(components[0], components[1], components[2]);
    if constexpr (size == 4)
        return HlslVector<T, 4>(components[0], components[1], components[2], components[3]);
}

template<typename T, size_t size, typename = std::enable_if_t<std::is_arithmetic_v<T> && 2 <= size && size <= 4>>
class RawVector // NOSONAR - class has more than 35 methods
{
public:
    using ComponentType  = T;
    using RawVectorType  = RawVector<T, size>;
    using HlslVectorType = HlslVector<T, size>;

    static constexpr size_t Size = size;

    RawVector() = default;

    template<typename V, typename = std::enable_if_t<std::is_arithmetic_v<V>>>
    RawVector(V) = delete;

    template<typename ...TArgs, typename = std::enable_if_t<std::conjunction_v<std::is_arithmetic<TArgs>...>>>
    RawVector(TArgs... args) noexcept : m_components{ RoundCast<T>(args)... } { } // NOSONAR - do not use explicit

    explicit RawVector(const std::array<T, size>& components) : m_components(components) { }
    explicit RawVector(std::array<T, size>&& components) : m_components(std::move(components)) { }
    explicit RawVector(const T* components_ptr) { std::copy_n(components_ptr, size, m_components.data()); }

    template<typename V, size_t sz = size, typename = std::enable_if_t<sz == 3>>
    RawVector(const RawVector<V, 2>& other, V z) noexcept
        : m_components{ RoundCast<T>(other.GetX()), RoundCast<T>(other.GetY()), RoundCast<T>(z) }
    { }

    template<typename V, size_t sz = size, typename = std::enable_if_t<sz == 4>>
    RawVector(const RawVector<V, 2>& other, V z, V w) noexcept
        : m_components{ RoundCast<T>(other.GetX()), RoundCast<T>(other.GetY()), RoundCast<T>(z), RoundCast<T>(w) }
    { }

    template<typename V, size_t sz = size, typename = std::enable_if_t<sz == 4>>
    RawVector(const RawVector<V, 3>& other, V w) noexcept
        : m_components{ RoundCast<T>(other.GetX()), RoundCast<T>(other.GetY()), RoundCast<T>(other.GetZ()), RoundCast<T>(w) } { }

    template<size_t sz = size, typename = std::enable_if_t<sz == 2>>
    explicit RawVector(const HlslVector<T, 2>& vec) noexcept: m_components{ vec.x, vec.y } { }

    template<size_t sz = size, typename = std::enable_if_t<sz == 3>>
    explicit RawVector(const HlslVector<T, 3>& vec) noexcept : m_components{ vec.x, vec.y, vec.z } { }

    template<size_t sz = size, typename = std::enable_if_t<sz == 4>>
    explicit RawVector(const HlslVector<T, 4>& vec) noexcept : m_components{ vec.x, vec.y, vec.z, vec.w } { }

    template<typename V, typename = std::enable_if_t<!std::is_same_v<T, V>>>
    explicit operator RawVector<V, size>() const noexcept
    {
        if constexpr (size == 2)
            return RawVector<V, 2>(RoundCast<V>(GetX()), RoundCast<V>(GetY()));
        if constexpr (size == 3)
            return RawVector<V, 3>(RoundCast<V>(GetX()), RoundCast<V>(GetY()), RoundCast<V>(GetZ()));
        if constexpr (size == 4)
            return RawVector<V, 4>(RoundCast<V>(GetX()), RoundCast<V>(GetY()), RoundCast<V>(GetZ()), RoundCast<V>(GetW()));
    }

    explicit operator std::string() const noexcept
    {
        if constexpr (size == 2)
            return fmt::format("V({}, {})", GetX(), GetY());
        if constexpr (size == 3)
            return fmt::format("V({}, {}, {})", GetX(), GetY(), GetZ());
        if constexpr (size == 4)
            return fmt::format("V({}, {}, {}, {})", GetX(), GetY(), GetZ(), GetW());
    }

    template<size_t sz = size, typename = std::enable_if_t<sz == 2>>
    [[nodiscard]] HlslVector<T, 2> AsHlsl() const noexcept { return HlslVector<T, 2>(GetX(), GetY()); }

    template<size_t sz = size, typename = std::enable_if_t<sz == 3>>
    [[nodiscard]] HlslVector<T, 3> AsHlsl() const noexcept { return HlslVector<T, 3>(GetX(), GetY(), GetZ()); }

    template<size_t sz = size, typename = std::enable_if_t<sz == 4>>
    [[nodiscard]] HlslVector<T, 4> AsHlsl() const noexcept { return HlslVector<T, 4>(GetX(), GetY(), GetZ(), GetW()); }

    [[nodiscard]] friend bool operator==(const RawVectorType& left, const RawVectorType& right) noexcept
    {
        if (left.m_components[0] != right[0])
        {
            return false;
        }
        if (left.m_components[1] != right[1])
        {
            return false;
        }
        if constexpr (size > 2)
        {
            if (left.m_components[2] != right[2])
                return false;
        }
        if constexpr (size > 3)
        {
            if (left.m_components[3] != right[3])
                return false;
        }
        return true;
    }

    [[nodiscard]] friend bool operator!=(const RawVectorType& left, const RawVectorType& right) noexcept { return !(left == right); }

    RawVectorType& operator*=(T multiplier) noexcept
    {
        UnrollUpdateComponents([multiplier](T& component, size_t) { component *= multiplier; });
        return *this;
    }

    RawVectorType& operator/=(T divisor) noexcept
    {
        UnrollUpdateComponents([divisor](T& component, size_t) { component /= divisor; });
        return *this;
    }

    [[nodiscard]] friend RawVectorType operator*(const RawVectorType& v, T multiplier) noexcept
    {
        return UnrollComputeComponents(v, [multiplier](T component, size_t) { return component * multiplier; });
    }

    [[nodiscard]] friend RawVectorType operator/(const RawVectorType& v, T divisor) noexcept
    {
        return UnrollComputeComponents(v, [divisor](T component, size_t) { return component / divisor; });
    }

    RawVectorType& operator+=(const RawVectorType& other) noexcept
    {
        UnrollUpdateComponents([&other](T& component, size_t i) { component += other[i]; });
        return *this;
    }

    RawVectorType& operator-=(const RawVectorType& other) noexcept
    {
        UnrollUpdateComponents([&other](T& component, size_t i) { component -= other[i]; });
        return *this;
    }

    [[nodiscard]] friend RawVectorType operator+(const RawVectorType& v, const RawVectorType& other) noexcept
    {
        return UnrollComputeComponents(v, [&other](T component, size_t i) { return component + other[i]; });
    }

    [[nodiscard]] friend RawVectorType operator-(const RawVectorType& v, const RawVectorType& other) noexcept
    {
        return UnrollComputeComponents(v, [&other](T component, size_t i) { return component - other[i]; });
    }

    [[nodiscard]] T GetLength() const noexcept
    {
        T square_sum{};
        ForEachComponent([&square_sum](T component) { square_sum += component * component; });
        return RoundCast<T>(std::sqrt(square_sum));
    }

    [[nodiscard]] T operator[](size_t index) const { META_CHECK_LESS(index, size); return m_components[index]; }
    [[nodiscard]] T& operator[](size_t index)      { META_CHECK_LESS(index, size); return m_components[index]; }

    [[nodiscard]] T Get(size_t index) const        { META_CHECK_LESS(index, size); return m_components[index]; }

    [[nodiscard]] T GetX() const noexcept { return m_components[0]; }
    [[nodiscard]] T GetY() const noexcept { return m_components[1]; }

    template<size_t sz = size, typename = std::enable_if_t<sz >= 3>>
    [[nodiscard]] T GetZ() const noexcept { return m_components[2]; }

    template<size_t sz = size, typename = std::enable_if_t<sz >= 4>>
    [[nodiscard]] T GetW() const noexcept { return m_components[3]; }

    RawVectorType& Set(size_t index, T c) { META_CHECK_LESS(index, size); m_components[index] = c; return *this; }

    RawVectorType& SetX(T x) noexcept { m_components[0] = x; return *this; }
    RawVectorType& SetY(T y) noexcept { m_components[1] = y; return *this; }

    template<size_t sz = size, typename = std::enable_if_t<sz >= 3>>
    RawVectorType& SetZ(T z) noexcept { m_components[2] = z; return *this; }

    template<size_t sz = size, typename = std::enable_if_t<sz >= 4>>
    RawVectorType& SetW(T w) noexcept { m_components[3] = w; return *this; }

private:
    template<typename ComponentFn /* [](T component) -> void */>
    static constexpr void ForEachComponent(const RawVectorType& v, ComponentFn component_action) noexcept
    {
        component_action(v.m_components[0]);
        component_action(v.m_components[1]);
        if constexpr (size >= 3)
            component_action(v.m_components[2]);
        if constexpr (size == 4)
            component_action(v.m_components[3]);
    }

    template<typename ComponentFn /* [](T component) -> void */>
    constexpr void ForEachComponent(ComponentFn component_action) const noexcept
    {
        ForEachComponent(*this, component_action);
    }

    template<typename ComponentFn /* [](T& component, size_t index) -> void */>
    static constexpr void UnrollUpdateComponents(RawVectorType& v, ComponentFn component_modifier) noexcept
    {
        component_modifier(v.m_components[0], 0);
        component_modifier(v.m_components[1], 1);
        if constexpr (size >= 3)
            component_modifier(v.m_components[2], 2);
        if constexpr (size == 4)
            component_modifier(v.m_components[3], 3);
    }

    template<typename ComponentFn /* [](T& component, size_t index) -> void */>
    constexpr void UnrollUpdateComponents(ComponentFn component_modifier) noexcept
    {
        UnrollUpdateComponents(*this, component_modifier);
    }

    template<typename ComponentFn /* [](T component, size_t index) -> T */>
    static constexpr RawVectorType UnrollComputeComponents(const RawVectorType& v, ComponentFn component_compute) noexcept
    {
        RawVectorType result;
        result[0] = component_compute(v.m_components[0], 0);
        result[1] = component_compute(v.m_components[1], 1);
        if constexpr (size >= 3)
            result[2] = component_compute(v.m_components[2], 2);
        if constexpr (size == 4)
            result[3] = component_compute(v.m_components[3], 3);
        return result;
    }

    template<typename ComponentFn /* [](T component, size_t index) -> T */>
    constexpr RawVectorType UnrollComputeComponents(ComponentFn component_compute) const noexcept
    {
        return UnrollComputeComponents(*this, component_compute);
    }

    std::array<T, size> m_components{{ }};

    static_assert(sizeof(m_components) == sizeof(T) * size,
                  "RawVector components raw array does not satisfy to dense memory packing requirement.");
};

using RawVector2F = RawVector<float, 2>;
using RawVector3F = RawVector<float, 3>;
using RawVector4F = RawVector<float, 4>;

} // namespace Methane::Data