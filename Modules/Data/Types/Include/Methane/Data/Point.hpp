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

FILE: Methane/Data/Point2D.hpp
2D Point type based on cml::vector

******************************************************************************/

#pragma once

#include <cml/vector.h>
#include <fmt/format.h>

#include <string>
#include <cstdint>

namespace Methane::Data
{

template<typename T, size_t vector_size, typename = std::enable_if_t<std::is_arithmetic_v<T> && 2 <= vector_size && vector_size <= 4>>
class PointT : public cml::vector<T, cml::fixed<vector_size>>
{
public:
    using CoordinateType = T;
    using VectorType = cml::vector<T, cml::fixed<vector_size>>;
    using PointType = PointT<T, vector_size>;

    using VectorType::VectorType;
    using VectorType::length;
    using VectorType::length_squared;
    using VectorType::normalize;

    template<typename V>
    explicit PointT(const PointT<V, vector_size>& other) : VectorType(other.AsVector()) { }

    explicit PointT(const VectorType& v) : VectorType(v) { }

    VectorType& AsVector() noexcept               { return *this; }
    const VectorType& AsVector() const noexcept   { return *this; }

    T GetX() const noexcept { return (*this)[0]; }
    T GetY() const noexcept { return (*this)[1]; }

    template<size_t sz = vector_size, typename = std::enable_if_t<sz >= 3, void>>
    T GetZ() const noexcept { return (*this)[2]; }

    template<size_t sz = vector_size, typename = std::enable_if_t<sz >= 4, void>>
    T GetW() const noexcept { return (*this)[3]; }

    void SetX(T x) noexcept { (*this)[0] = x; }
    void SetY(T y) noexcept { (*this)[1] = y; }

    template<size_t sz = vector_size, typename = std::enable_if_t<sz >= 3, void>>
    void SetZ(T z) noexcept { return (*this)[2] = z; }

    template<size_t sz = vector_size, typename = std::enable_if_t<sz >= 4, void>>
    void SetW(T w) noexcept { return (*this)[3] = w; }

    bool operator==(const PointType& other) const noexcept
    { return static_cast<const VectorType&>(*this) == static_cast<const VectorType&>(other); }

    bool operator!=(const PointType& other) const noexcept
    { return static_cast<const VectorType&>(*this) != static_cast<const VectorType&>(other); }

    PointType operator+(const PointType& other) const noexcept
    {
        if constexpr (vector_size == 2)
            return PointType(GetX() + other.GetX(), GetY() + other.GetY());
        else if constexpr (vector_size == 3)
            return PointType(GetX() + other.GetX(), GetY() + other.GetY(), GetZ() + other.GetZ());
        else if constexpr (vector_size == 4)
            return PointType(GetX() + other.GetX(), GetY() + other.GetY(), GetZ() + other.GetZ(), GetW() + other.GetW());
    }

    PointType operator-(const PointType& other) const noexcept
    {
        if constexpr (vector_size == 2)
            return PointType(GetX() - other.GetX(), GetY() - other.GetY());
        else if constexpr (vector_size == 3)
            return PointType(GetX() - other.GetX(), GetY() - other.GetY(), GetZ() - other.GetZ());
        else if constexpr (vector_size == 4)
            return PointType(GetX() - other.GetX(), GetY() - other.GetY(), GetZ() - other.GetZ(), GetW() - other.GetW());
    }

    PointType& operator+=(const PointType& other) noexcept
    {
        for(int comp_index = 0; comp_index < static_cast<int>(vector_size); ++comp_index)
        {
            (*this)[comp_index] = (*this)[comp_index] + other[comp_index];
        }
        return *this;
    }

    PointType& operator-=(const PointType& other) noexcept
    {
        for(int comp_index = 0; comp_index < static_cast<int>(vector_size); ++comp_index)
        {
            (*this)[comp_index] = (*this)[comp_index] - other[comp_index];
        }
        return *this;
    }

    template<typename M, typename = std::enable_if_t<std::is_arithmetic_v<M>>>
    PointType operator*(M multiplier) const noexcept
    {
        if constexpr (vector_size == 2)
            return PointType(Multiply(GetX(), multiplier), Multiply(GetY(), multiplier));
        else if constexpr (vector_size == 3)
            return PointType(Multiply(GetX(), multiplier), Multiply(GetY(), multiplier), Multiply(GetZ(), multiplier));
        else if constexpr (vector_size == 4)
            return PointType(Multiply(GetX(), multiplier), Multiply(GetY(), multiplier), Multiply(GetZ(), multiplier), Multiply(GetW(), multiplier));
    }

    template<typename M, typename = std::enable_if_t<std::is_arithmetic_v<M>>>
    PointType operator/(M divisor) const noexcept
    {
        if constexpr (vector_size == 2)
            return PointType(Divide(GetX(), divisor), Divide(GetY(), divisor));
        else if constexpr (vector_size == 3)
            return PointType(Divide(GetX(), divisor), Divide(GetY(), divisor), Divide(GetZ(), divisor));
        else if constexpr (vector_size == 4)
            return PointType(Divide(GetX(), divisor), Divide(GetY(), divisor), Divide(GetZ(), divisor), Divide(GetW(), divisor));
    }

    template<typename M>
    PointType operator*(const PointT<M, vector_size>& multiplier) const noexcept
    {
        if constexpr (vector_size == 2)
            return PointType(Multiply(GetX(), multiplier.GetX()), Multiply(GetY(), multiplier.GetY()));
        else if constexpr (vector_size == 3)
            return PointType(Multiply(GetX(), multiplier.GetX()), Multiply(GetY(), multiplier.GetY()), Multiply(GetZ(), multiplier.GetZ()));
        else if constexpr (vector_size == 4)
            return PointType(Multiply(GetX(), multiplier.GetX()), Multiply(GetY(), multiplier.GetY()), Multiply(GetZ(), multiplier.GetZ()), Multiply(GetW(), multiplier.GetZ()));
    }

    template<typename M>
    PointType operator/(const PointT<M, vector_size>& divisor) const noexcept
    {
        if constexpr (vector_size == 2)
            return PointType(Divide(GetX(), divisor.GetX()), Divide(GetY(), divisor.GetY()));
        else if constexpr (vector_size == 3)
            return PointType(Divide(GetX(), divisor.GetX()), Divide(GetY(), divisor.GetY()), Divide(GetZ(), divisor.GetZ()));
        else if constexpr (vector_size == 4)
            return PointType(Divide(GetX(), divisor.GetX()), Divide(GetY(), divisor.GetY()), Divide(GetZ(), divisor.GetZ()), Divide(GetW(), divisor.GetZ()));
    }

    template<typename M, typename = std::enable_if_t<std::is_arithmetic_v<M>>>
    PointType& operator*=(M multiplier) noexcept
    {
        for(int comp_index = 0; comp_index < static_cast<int>(vector_size); ++comp_index)
        {
            (*this)[comp_index] = Multiply((*this)[comp_index], multiplier);
        }
        return *this;
    }

    template<typename M, typename = std::enable_if_t<std::is_arithmetic_v<M>>>
    PointType& operator/=(M divisor) noexcept
    {
        for(int comp_index = 0; comp_index < static_cast<int>(vector_size); ++comp_index)
        {
            (*this)[comp_index] = Divide((*this)[comp_index], divisor);
        }
        return *this;
    }

    template<typename M>
    PointType& operator*=(const PointT<M, vector_size>& multiplier) noexcept
    {
        for(int comp_index = 0; comp_index < static_cast<int>(vector_size); ++comp_index)
        {
            (*this)[comp_index] = Multiply((*this)[comp_index], multiplier[comp_index]);
        }
        return *this;
    }

    template<typename M>
    PointType& operator/=(const PointT<M, vector_size>& divisor) noexcept
    {
        for(int comp_index = 0; comp_index < static_cast<int>(vector_size); ++comp_index)
        {
            (*this)[comp_index] = Divide((*this)[comp_index], divisor[comp_index]);
        }
        return *this;
    }

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

    explicit operator VectorType() const noexcept { return *this; }

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
    template<typename M> T Multiply(T t, M m) const noexcept { return static_cast<T>(static_cast<M>(t) * m); }
    template<typename M> T Divide(T t, M m) const noexcept   { return static_cast<T>(static_cast<M>(t) / m); }
};

template<typename T>
using Point2T = PointT<T, 2>;

using Point2i = Point2T<int32_t>;
using Point2u = Point2T<uint32_t>;
using Point2f = Point2T<float>;
using Point2d = Point2T<double>;

} // namespace Methane::Data
