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

FILE: Methane/Graphics/Color.hpp
Color wrapper type based on HLSL++ vector.

******************************************************************************/

#pragma once

#include <Methane/Data/Vector.hpp>
#include <Methane/Checks.hpp>

#include <fmt/format.h>
#include <string_view>
#include <array>
#include <cstdint>

namespace Methane::Graphics
{

template<typename T, size_t size, typename = std::enable_if_t<3 <= size && size <= 4>>
class Color
{
public:
    using ComponentType = T;
    using VectorType = Data::HlslVector<ComponentType, size>;
    static constexpr size_t Size = size;

    Color() = default;

    template<typename... Args, typename = std::enable_if_t<std::conjunction_v<std::is_arithmetic<Args>...>>>
    Color(Args... args) // NOSONAR - do not use explicit
        : m_components(ComponentCast<T>(args)...)
    {
        CheckComponentsRange();
    }

    template<size_t sz = size, typename = std::enable_if_t<sz < size>>
    Color(const Color<T, sz>& color, T a)
        : m_components(color.AsVector(), a)
    {
        CheckComponentsRange();
    }

    explicit Color(const std::array<T, size>& components)
        : m_components(Data::CreateHlslVector(components))
    {
        CheckComponentsRange();
    }

    explicit Color(const VectorType& components)
        : m_components(components)
    {
        CheckComponentsRange();
    }

    explicit Color(VectorType&& components)
        : m_components(std::move(components))
    {
        CheckComponentsRange();
    }

    [[nodiscard]] friend bool operator==(const Color& left, const Color& right) noexcept
    {
        return hlslpp::all(left.m_components == right.m_components);
    }

    [[nodiscard]] size_t GetSize() const noexcept { return Size; }

    template<typename V = T>
    [[nodiscard]] V GetRed() const noexcept   { return ComponentCast<V, T>(m_components.r); }

    template<typename V = T>
    [[nodiscard]] V GetGreen() const noexcept { return ComponentCast<V, T>(m_components.g); }

    template<typename V = T>
    [[nodiscard]] V GetBlue() const noexcept  { return ComponentCast<V, T>(m_components.b); }

    template<typename V = T, size_t sz = size, typename = std::enable_if_t<sz == 4>>
    [[nodiscard]] V GetAlpha() const noexcept { return ComponentCast<V, T>(m_components.a); }

    template<typename V = T>
    Color& SetRed(V r)
    {
        CheckComponentRange(r, "Red");
        m_components.r = ComponentCast<T, V>(r);
        return *this;
    }

    template<typename V = T>
    Color& SetGreen(V g)
    {
        CheckComponentRange(g, "Green");
        m_components.g = ComponentCast<T, V>(g);
        return *this;
    }

    template<typename V = T>
    Color& SetBlue(V b)
    {
        CheckComponentRange(b, "Blue");
        m_components.b = ComponentCast<T, V>(b);
        return *this;
    }

    template<typename V = T, size_t sz = size, typename = std::enable_if_t<sz == 4>>
    Color& SetAlpha(V a)
    {
        CheckComponentRange(a, "Alpha");
        m_components.a = ComponentCast<T, V>(a);
        return *this;
    }

    template<typename V = T>
    [[nodiscard]] V Get(size_t component_index) const
    {
        META_CHECK_LESS(component_index, Size);
        switch(component_index)
        {
        case 0: return GetRed<V>();
        case 1: return GetGreen<V>();
        case 2: return GetBlue<V>();
        case 3: if constexpr (size == 4) return GetAlpha<V>();
        default: META_UNEXPECTED_RETURN(component_index, V{});
        }
    }

    template<typename V = T>
    Color& Set(size_t component_index, V value)
    {
        META_CHECK_LESS(component_index, Size);
        switch(component_index)
        {
        case 0: SetRed<V>(value); break;
        case 1: SetGreen<V>(value); break;
        case 2: SetBlue<V>(value); break;
        case 3: if constexpr (size == 4) { SetAlpha<V>(value); } break;
        default: META_UNEXPECTED(component_index);
        }
        return *this;
    }

    [[nodiscard]] T operator[](size_t component_index) const { return Get(component_index); }

    [[nodiscard]] explicit operator VectorType() const noexcept { return m_components; }
    [[nodiscard]] const VectorType& AsVector() const noexcept   { return m_components; }

    template<typename V = T>
    [[nodiscard]] std::array<V, size> AsArray() const noexcept
    {
        if constexpr (size == 4)
            return { GetRed<V>(), GetGreen<V>(), GetBlue<V>(), GetAlpha<V>() };
        else
            return { GetRed<V>(), GetGreen<V>(), GetBlue<V>() };
    }

    template<typename V>
    [[nodiscard]] explicit operator Color<V, size>() const
    {
        if constexpr (size == 3)
            return Color<V, 3>(GetRed(), GetGreen(), GetBlue());
        else
            return Color<V, 4>(GetRed(), GetGreen(), GetBlue(), GetAlpha());
    }

    [[nodiscard]] explicit operator std::string() const noexcept
    {
        if constexpr (size == 3)
            return fmt::format("C(r:{:d}, g:{:d}, b:{:d})",
                               GetRed<uint8_t>(), GetGreen<uint8_t>(), GetBlue<uint8_t>());
        else
            return fmt::format("C(r:{:d}, g:{:d}, b:{:d}, a:{:d})",
                               GetRed<uint8_t>(), GetGreen<uint8_t>(), GetBlue<uint8_t>(), GetAlpha<uint8_t>());
    }

    template<typename V = T>
    static constexpr std::pair<V, V> GetComponentRange()
    {
        if constexpr (std::is_floating_point_v<V>)
            return { V(0), V(1) };
        else
            return { V(0), std::numeric_limits<V>::max() };
    }

    template<typename V = T>
    static constexpr V GetComponentMax()
    {
        if constexpr (std::is_floating_point_v<V>)
            return V(1);
        else
            return std::numeric_limits<V>::max();
    }

private:
    template<typename V, typename S>
    static constexpr V ComponentCast(S component) noexcept
    {
        if constexpr (std::is_same_v<S, V>)
        {
            return component;
        }
        else
        {
            if constexpr (std::is_floating_point_v<S>)
            {
                if constexpr (std::is_floating_point_v<V>)
                    return static_cast<V>(component);
                else
                    return Data::RoundCast<V>(component * static_cast<S>(GetComponentMax<V>()));
            }
            else
            {
                if constexpr (std::is_floating_point_v<V>)
                    return static_cast<V>(component) / static_cast<V>(GetComponentMax<S>());
                else
                    return Data::RoundCast<V>(static_cast<double>(component) * static_cast<double>(GetComponentMax<V>()) / static_cast<double>(GetComponentMax<S>()));
            }
        }
    }

    template<typename V>
    static void CheckComponentRange(V component, std::string_view name)
    {
        static const std::pair<V, V> s_component_range = GetComponentRange<V>();
        META_CHECK_RANGE_INC_DESCR(component, s_component_range.first, s_component_range.second, "for {} color component", name);
        META_UNUSED(name);
    }

    void CheckComponentsRange()
    {
        CheckComponentRange<T>(m_components.r, "Red");
        CheckComponentRange<T>(m_components.g, "Green");
        CheckComponentRange<T>(m_components.b, "Blue");
        if constexpr (size == 4)
            CheckComponentRange<T>(m_components.a, "Alpha");
    }

    VectorType m_components;
};

template<size_t size>
using ColorF = Color<float, size>;
using Color3F = ColorF<3>;
using Color4F = ColorF<4>;

template<size_t size>
using ColorB = Color<uint8_t, size>;
using Color3B = ColorB<3>;
using Color4B = ColorB<4>;

template<size_t size>
using ColorU  = Color<uint32_t, size>;
using Color3U = ColorU<3>;
using Color4U = ColorU<4>;

} // namespace Methane::Graphics
