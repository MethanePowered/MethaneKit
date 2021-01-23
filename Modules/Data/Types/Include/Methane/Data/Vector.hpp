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

#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>

#include <hlsl++.h>
#include <fmt/format.h>
#include <string>

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
    using ComponentType  = T;
    using RawVectorType  = RawVector<T, size>;
    using HlslVectorType = HlslVector<T, size>;

    static constexpr size_t Size = size;

    RawVector() = default;

    template<typename V>
    RawVector(V) = delete;

    template<typename ...TArgs>
    RawVector(TArgs... args) noexcept : m_components{ static_cast<T>(args)... } { } // NOSONAR - do not use explicit

    explicit RawVector(const T* components_ptr) { std::copy_n(components_ptr, size, m_components); }

    template<size_t sz, typename ...TArgs, typename = std::enable_if_t<sz == 2>>
    RawVector(const RawVector<T, sz>& other, T z, T w) noexcept : m_components{ other.GetX(), other.GetY(), z, w } { }

    template<size_t sz, typename ...TArgs, typename = std::enable_if_t<sz == 3>>
    RawVector(const RawVector<T, sz>& other, T w) noexcept : m_components{ other.GetX(), other.GetY(), other.GetZ(), w } { }

    template<size_t sz = size, typename = std::enable_if_t<sz == 2>>
    explicit RawVector(const HlslVector<T, 2>& vec) noexcept: m_components{ vec.x, vec.y } { }

    template<size_t sz = size, typename = std::enable_if_t<sz == 3>>
    explicit RawVector(const HlslVector<T, 3>& vec) noexcept : m_components{ vec.x, vec.y, vec.z } { }

    template<size_t sz = size, typename = std::enable_if_t<sz == 4>>
    explicit RawVector(const HlslVector<T, 4>& vec) noexcept : m_components{ vec.x, vec.y, vec.z, vec.w } { }

    template<typename V>
    explicit operator RawVector<V, size>() const noexcept
    {
        if constexpr (size == 2)
            return RawVector<V, 2>(static_cast<V>(GetX()), static_cast<V>(GetY()));
        if constexpr (size == 3)
            return RawVector<V, 3>(static_cast<V>(GetX()), static_cast<V>(GetY()), static_cast<V>(GetZ()));
        if constexpr (size == 4)
            return RawVector<V, 4>(static_cast<V>(GetX()), static_cast<V>(GetY()), static_cast<V>(GetZ()), static_cast<V>(GetW()));
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

    [[nodiscard]] bool operator==(const RawVectorType& other) const noexcept
    {
        for(size_t i = 0; i < size; ++i)
            if (m_components[i] != other[i])
                return false;
        return true;
    }

    [[nodiscard]] bool operator!=(const RawVectorType& other) const noexcept { return !operator==(other); }

    RawVectorType& operator*=(T multiplier) noexcept
    {
        for(size_t i = 0; i < size; ++i)
            m_components[i] *= multiplier;
        return *this;
    }

    RawVectorType& operator/=(T multiplier) noexcept
    {
        for(size_t i = 0; i < size; ++i)
            m_components[i] *= multiplier;
        return *this;
    }

    [[nodiscard]] RawVectorType operator*(T multiplier) const noexcept
    {
        RawVectorType result;
        for(size_t i = 0; i < size; ++i)
            result[i] = m_components[i] * multiplier;
        return result;
    }

    [[nodiscard]] RawVectorType operator/(T divisor) const noexcept
    {
        RawVectorType result;
        for(size_t i = 0; i < size; ++i)
            result[i] = m_components[i] / divisor;
        return result;
    }

    RawVectorType& operator+=(const RawVectorType& other) noexcept
    {
        for(size_t i = 0; i < size; ++i)
            m_components[i] += other[i];
        return *this;
    }

    RawVectorType& operator-=(const RawVectorType& other) noexcept
    {
        for(size_t i = 0; i < size; ++i)
            m_components[i] -= other[i];
        return *this;
    }

    [[nodiscard]] RawVectorType operator+(const RawVectorType& other) const noexcept
    {
        RawVectorType result;
        for(size_t i = 0; i < size; ++i)
            result[i] = m_components[i] + other[i];
        return result;
    }

    [[nodiscard]] RawVectorType operator-(const RawVectorType& other) const noexcept
    {
        RawVectorType result;
        for(size_t i = 0; i < size; ++i)
            result[i] = m_components[i] - other[i];
        return result;
    }

    [[nodiscard]] T GetLength() const noexcept
    {
        T square_sum{};
        for(size_t i = 0; i < size; ++i)
            square_sum += m_components[i];
        return std::sqrt(square_sum);
    }

    [[nodiscard]] T operator[](size_t index) const noexcept { return m_components[index]; }
    [[nodiscard]] T& operator[](size_t index) noexcept      { return m_components[index]; }

    [[nodiscard]] T Get(size_t index) const { META_CHECK_ARG_LESS(index, size); return m_components[index]; }

    [[nodiscard]] T GetX() const noexcept { return m_components[0]; }
    [[nodiscard]] T GetY() const noexcept { return m_components[1]; }

    template<size_t sz = size, typename = std::enable_if_t<sz >= 3>>
    [[nodiscard]] T GetZ() const noexcept { return m_components[2]; }

    template<size_t sz = size, typename = std::enable_if_t<sz == 4>>
    [[nodiscard]] T GetW() const noexcept { return m_components[3]; }

    void SetX(T x) noexcept { m_components[0] = x; }
    void SetY(T y) noexcept { m_components[1] = y; }

    template<size_t sz = size, typename = std::enable_if_t<sz >= 3>>
    void SetZ(T z) noexcept { m_components[2] = z; }

    template<size_t sz = size, typename = std::enable_if_t<sz >= 4>>
    void SetW(T w) noexcept { m_components[3] = w; }

private:
    T m_components[size]{ };
};

using RawVector2F = RawVector<float, 2>;
using RawVector3F = RawVector<float, 3>;
using RawVector4F = RawVector<float, 4>;

template<typename RawVectorType>
using HlslVectorType = typename RawVectorType::HlslVectorType;

} // namespace Methane::Data