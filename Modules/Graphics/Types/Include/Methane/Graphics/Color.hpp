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
Color type based on cml::vector.

******************************************************************************/

#pragma once

#include <Methane/Data/Types.h>
#include <Methane/Data/Vector.hpp>
#include <Methane/Checks.hpp>

#include <fmt/format.h>
#include <cstdint>

namespace Methane::Graphics
{

template<Data::Size vector_size, typename = std::enable_if_t<3 <= vector_size && vector_size <= 4>>
class ColorF
{
public:
    using VectorType = Data::VectorType<float, vector_size>;

    static constexpr Data::Size Size = vector_size;

    ColorF() = default;
    ColorF(float r, float g, float b) noexcept : m_components(r, g, b) { }

    template<Data::Size sz = vector_size, typename = std::enable_if_t<sz == 4>>
    ColorF(float r, float g, float b, float a) noexcept : m_components(r, g, b, a) { }

    template<Data::Size sz = vector_size, typename = std::enable_if_t<sz == 4>>
    ColorF(const ColorF<3>& color, float a) noexcept : m_components(color.AsVector(), a) { }

    template<typename... Args>
    explicit ColorF(const VectorType& components, Args&&... args) noexcept : m_components(components, std::forward<Args>(args)...) { }

    explicit ColorF(VectorType&& components) noexcept : m_components(std::move(components)) { }

    [[nodiscard]] bool operator==(const ColorF& other) const noexcept  { return hlslpp::all(m_components == other.m_components); }
    [[nodiscard]] bool operator!=(const ColorF& other) const noexcept  { return hlslpp::any(m_components != other.m_components); }

    [[nodiscard]] float operator[](Data::Index component_index) const
    {
        META_CHECK_ARG_LESS(component_index, Size);
        switch(component_index)
        {
        case 0: return m_components.r;
        case 1: return m_components.g;
        case 2: return m_components.b;
        case 3: if constexpr (vector_size == 4) return m_components.a;
        default: META_UNEXPECTED_ENUM_ARG_RETURN(component_index, 0.f);
        }
    }

    [[nodiscard]] explicit operator VectorType() const noexcept { return m_components; }
    [[nodiscard]] const VectorType& AsVector() const noexcept { return m_components; }

    [[nodiscard]] Data::Size GetSize() const noexcept { return Size; }

    [[nodiscard]] float GetRf() const noexcept { return m_components.r; }
    [[nodiscard]] float GetGf() const noexcept { return m_components.g; }
    [[nodiscard]] float GetBf() const noexcept { return m_components.b; }

    template<size_t sz = vector_size, typename = std::enable_if_t<sz == 4>>
    [[nodiscard]] float GetAf() const noexcept { return m_components.a; }

    [[nodiscard]] float GetNormRf() const noexcept { return GetNormColorComponent(GetRf()); }
    [[nodiscard]] float GetNormGf() const noexcept { return GetNormColorComponent(GetGf()); }
    [[nodiscard]] float GetNormBf() const noexcept { return GetNormColorComponent(GetBf()); }

    template<size_t sz = vector_size, typename = std::enable_if_t<sz == 4>>
    [[nodiscard]] float GetNormAf() const noexcept { return GetNormColorComponent(GetAf()); }

    [[nodiscard]] uint8_t GetRu() const noexcept { return GetUintColorComponent(GetNormRf()); }
    [[nodiscard]] uint8_t GetGu() const noexcept { return GetUintColorComponent(GetNormGf()); }
    [[nodiscard]] uint8_t GetBu() const noexcept { return GetUintColorComponent(GetNormBf()); }

    template<size_t sz = vector_size, typename = std::enable_if_t<sz == 4>>
    [[nodiscard]] uint8_t GetAu() const noexcept { return GetUintColorComponent(GetNormAf()); }

    void Set(Data::Index component_index, float value)
    {
        META_CHECK_ARG_LESS(component_index, Size);
        META_CHECK_ARG_RANGE(value, s_float_range.first, s_float_range.second);
        switch(component_index)
        {
        case 0: m_components.r = value; break;
        case 1: m_components.g = value; break;
        case 2: m_components.b = value; break;
        case 3: if constexpr (vector_size == 4) { m_components.a = value; break; }
        default: META_UNEXPECTED_ENUM_ARG(component_index);
        }
    }

    void SetR(float r) { META_CHECK_ARG_RANGE(r, s_float_range.first, s_float_range.second); m_components.r = r; }
    void SetG(float g) { META_CHECK_ARG_RANGE(g, s_float_range.first, s_float_range.second); m_components.g = g; }
    void SetB(float b) { META_CHECK_ARG_RANGE(b, s_float_range.first, s_float_range.second); m_components.b = b; }

    template<size_t sz = vector_size, typename = std::enable_if_t<sz == 4>>
    void SetA(float a) { META_CHECK_ARG_RANGE(a, s_float_range.first, s_float_range.second); m_components.a = a; }

    void Set(Data::Index component_index, uint8_t value) { Set(component_index, GetFloatColorComponent(value)); }
    void SetR(uint8_t r) { m_components.r = GetFloatColorComponent(r); }
    void SetG(uint8_t g) { m_components.g = GetFloatColorComponent(g); }
    void SetB(uint8_t b) { m_components.b = GetFloatColorComponent(b); }

    template<size_t sz = vector_size, typename = std::enable_if_t<sz == 4>>
    void SetA(uint8_t a) { m_components.a = GetFloatColorComponent(a); }

    [[nodiscard]] explicit operator std::string() const noexcept
    {
        if constexpr (vector_size == 3)
            return fmt::format("C(r:{:d}, g:{:d}, b:{:d})", GetRu(), GetGu(), GetBu());
        else
            return fmt::format("C(r:{:d}, g:{:d}, b:{:d}, a:{:a})", GetRu(), GetGu(), GetBu(), GetAu());
    }

private:
    [[nodiscard]] inline float GetNormColorComponent(float component) const noexcept
    {
        return std::max(s_float_range.first, std::min(s_float_range.second, component));
    }

    [[nodiscard]] inline uint8_t GetUintColorComponent(float component) const noexcept
    {
        return static_cast<uint8_t>(std::round(component * static_cast<float>(s_uint_component_max)));
    }

    [[nodiscard]] inline float GetFloatColorComponent(uint8_t component) const
    {
        META_CHECK_ARG_RANGE(component, 0, s_uint_component_max);
        return static_cast<float>(component) / static_cast<float>(s_uint_component_max);
    }

    VectorType m_components;

    static constexpr std::pair<float, float> s_float_range{ 0.F, 1.F };
    static constexpr uint8_t s_uint_component_max = std::numeric_limits<uint8_t>::max();
};

using Color3f = ColorF<3>;
using Color4f = ColorF<4>;

} // namespace Methane::Graphics
