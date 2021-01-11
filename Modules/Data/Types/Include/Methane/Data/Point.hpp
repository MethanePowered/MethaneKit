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

FILE: Methane/Data/Point.hpp
2D Point type based on cml::vector

******************************************************************************/

#pragma once

#include "Vector.hpp"

#include <fmt/format.h>

#include <string>
#include <cstdint>

namespace Methane::Data
{

template<typename T, size_t vector_size, typename = std::enable_if_t<std::is_arithmetic_v<T> && 2 <= vector_size && vector_size <= 4>>
class PointT
{
public:
    using CoordinateType = T;
    using VectorType = typename Vector<T, vector_size>::Type;
    using PointType = PointT<T, vector_size>;

    static constexpr size_t dimensions_count = vector_size;

    PointT() = default;

    template<size_t sz = vector_size, typename = std::enable_if_t<sz == 2, void>>
    PointT(T x, T y) noexcept : m_vector(x, y) { }

    template<size_t sz = vector_size, typename = std::enable_if_t<sz == 3, void>>
    PointT(T x, T y, T z) noexcept : m_vector(x, y, z) { }

    template<size_t sz = vector_size, typename = std::enable_if_t<sz == 4, void>>
    PointT(T x, T y, T z, T w) noexcept : m_vector(x, y, z, w) { }

    explicit PointT(const VectorType& vector) noexcept : m_vector(vector) { }

    template<typename V, size_t sz = vector_size, typename = std::enable_if_t<sz == 2>, typename = std::enable_if_t<!std::is_same_v<T, V>>>
    explicit PointT(const PointT<V, 2>& other) noexcept
        : m_vector(static_cast<V>(other.GetX()), static_cast<V>(other.GetY()))
    { }

    template<typename V, size_t sz = vector_size, typename = std::enable_if_t<sz == 3>, typename = std::enable_if_t<!std::is_same_v<T, V>>>
    explicit PointT(const PointT<V, 3>& other) noexcept
        : m_vector(static_cast<V>(other.GetX()), static_cast<V>(other.GetY()), static_cast<V>(other.GetZ()))
    { }

    template<typename V, size_t sz = vector_size, typename = std::enable_if_t<sz == 4>, typename = std::enable_if_t<!std::is_same_v<T, V>>>
    explicit PointT(const PointT<V, 4>& other) noexcept
        : m_vector(static_cast<V>(other.GetX()), static_cast<V>(other.GetY()), static_cast<V>(other.GetZ()), static_cast<V>(other.GetW()))
    { }

    VectorType& AsVector() noexcept               { return m_vector; }
    const VectorType& AsVector() const noexcept   { return m_vector; }

    T GetX() const noexcept { return m_vector.x; }
    T GetY() const noexcept { return m_vector.y; }

    template<size_t sz = vector_size, typename = std::enable_if_t<sz >= 3, void>>
    T GetZ() const noexcept { return m_vector.z; }

    template<size_t sz = vector_size, typename = std::enable_if_t<sz >= 4, void>>
    T GetW() const noexcept { return m_vector.w; }

    void SetX(T x) noexcept { m_vector.x = x; }
    void SetY(T y) noexcept { m_vector.y = y; }

    template<size_t sz = vector_size, typename = std::enable_if_t<sz >= 3, void>>
    void SetZ(T z) noexcept { return m_vector.z = z; }

    template<size_t sz = vector_size, typename = std::enable_if_t<sz >= 4, void>>
    void SetW(T w) noexcept { return m_vector.w = w; }

    T GetLength() const noexcept { return hlslpp::length(m_vector); }
    T GetLengthSquared() const noexcept { const T len = GetLength(); return len * len; }

    PointType& Normalize() noexcept { m_vector = hlslpp::normalize(m_vector); return *this; }

    bool operator==(const PointType& other) const noexcept { return hlslpp::all(m_vector == other.AsVector()); }
    bool operator!=(const PointType& other) const noexcept { return hlslpp::any(m_vector != other.AsVector()); }
    bool operator<(const PointType& other) const noexcept  { return hlslpp::all(m_vector <  other.AsVector()); }
    bool operator<=(const PointType& other) const noexcept { return hlslpp::all(m_vector <= other.AsVector()); }
    bool operator>(const PointType& other) const noexcept  { return hlslpp::all(m_vector >  other.AsVector()); }
    bool operator>=(const PointType& other) const noexcept { return hlslpp::all(m_vector >= other.AsVector()); }

    PointType operator+(const PointType& other) const noexcept { return PointType(m_vector + other.AsVector()); }
    PointType operator-(const PointType& other) const noexcept { return PointType(m_vector - other.AsVector()); }
    PointType& operator+=(const PointType& other) noexcept     { m_vector += other.AsVector(); return *this; }
    PointType& operator-=(const PointType& other) noexcept     { m_vector -= other.AsVector(); return *this; }

    template<typename M, typename = std::enable_if_t<std::is_arithmetic_v<M>>>
    PointType operator*(M multiplier) const noexcept
    { return PointType(m_vector * ScalarCast(multiplier)); }

    template<typename M, typename = std::enable_if_t<std::is_arithmetic_v<M>>>
    PointType operator/(M divisor) const noexcept
    { return PointType(m_vector / ScalarCast(divisor)); }

    template<typename M>
    PointType operator*(const PointT<M, vector_size>& multiplier) const noexcept
    { return PointType(m_vector * static_cast<typename Vector<T, vector_size>::Type>(multiplier.AsVector())); }

    template<typename M>
    PointType operator/(const PointT<M, vector_size>& divisor) const noexcept
    { return PointType(m_vector / static_cast<typename Vector<T, vector_size>::Type>(divisor.AsVector())); }

    template<typename M, typename = std::enable_if_t<std::is_arithmetic_v<M>>>
    PointType& operator*=(M multiplier) noexcept
    { m_vector *= ScalarCast(multiplier); return *this; }

    template<typename M, typename = std::enable_if_t<std::is_arithmetic_v<M>>>
    PointType& operator/=(M divisor) noexcept
    { m_vector /= ScalarCast(divisor); return *this; }

    template<typename M>
    PointType& operator*=(const PointT<M, vector_size>& multiplier) noexcept
    { m_vector *= static_cast<typename Vector<T, vector_size>::Type>(multiplier.AsVector()); return *this; }

    template<typename M>
    PointType& operator/=(const PointT<M, vector_size>& divisor) noexcept
    { m_vector /= static_cast<typename Vector<T, vector_size>::Type>(divisor.AsVector()); return *this; }

    template<typename U>
    explicit operator PointT<U, vector_size>() const noexcept
    {
        if constexpr (vector_size == 2)
            return PointT<U, 2>(static_cast<U>(GetX()), static_cast<U>(GetY()));
        else if constexpr (vector_size == 3)
            return PointT<U, 3>(static_cast<U>(GetX()), static_cast<U>(GetY()), static_cast<U>(GetZ()));
        else if constexpr (vector_size == 4)
            return PointT<U, 4>(static_cast<U>(GetX()), static_cast<U>(GetY()), static_cast<U>(GetZ()), static_cast<U>(GetW()));
    }

    explicit operator VectorType() const noexcept { return m_vector; }

    explicit operator std::string() const
    {
        if constexpr (vector_size == 2)
            return fmt::format("P({}, {})", GetX(), GetY());
        else if constexpr (vector_size == 3)
            return fmt::format("P({}, {}, {})", GetX(), GetY(), GetZ());
        else if constexpr (vector_size == 4)
            return fmt::format("P({}, {}, {}, {})", GetX(), GetY(), GetZ(), GetW());
    }

private:
    template<typename M, typename = std::enable_if_t<std::is_arithmetic_v<M>>>
    T ScalarCast(M m) const noexcept
    {
        if constexpr (std::is_same_v<M, T>)
            return m;
        else
            return static_cast<T>(m);
    }

    VectorType m_vector;
};

template<typename T>
using Point2T = PointT<T, 2>;

using Point2i = Point2T<int32_t>;
using Point2u = Point2T<uint32_t>;
using Point2f = Point2T<float>;
using Point2d = Point2T<double>;

} // namespace Methane::Data
