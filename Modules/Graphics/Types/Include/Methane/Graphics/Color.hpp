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
#include <Methane/Checks.hpp>

#include <cml/vector.h>
#include <fmt/format.h>
#include <cstdint>

namespace Methane::Graphics
{

template<Data::Size vector_size, typename VectorType = std::enable_if_t<3 <= vector_size && vector_size <= 4, cml::vector<float, cml::fixed<vector_size>>>>
class ColorF
{
public:
    static constexpr Data::Size Size = vector_size;

    ColorF() noexcept = default;
    ColorF(float r, float g, float b) noexcept : m_components(r, g, b) { }

    template<Data::Size sz = vector_size, typename = std::enable_if_t<sz>= 4, void>>
    ColorF(float r, float g, float b, float a) noexcept : m_components(r, g, b, a) { }

    template<Data::Size sz = vector_size, typename = std::enable_if_t<sz>= 4, void>>
    ColorF(const ColorF<3>& color, float a) noexcept : m_components(color.AsVector(), a) { }

    template<typename... Args>
    explicit ColorF(const VectorType& components, Args&&... args) noexcept : m_components(components, std::forward<Args>(args)...) { }

    explicit ColorF(VectorType&& components) noexcept : m_components(std::move(components)) { }

    bool operator==(const ColorF& other) const noexcept  { return m_components == other.m_components; }
    bool operator!=(const ColorF& other) const noexcept  { return m_components != other.m_components; }
    float operator[](Data::Index component_index) const  { META_CHECK_ARG_LESS(component_index, Size); return m_components[static_cast<int>(component_index)]; }
    explicit operator VectorType() const noexcept { return m_components; }

    const VectorType& AsVector() const noexcept { return m_components; }

    Data::Size GetSize() const noexcept      { return vector_size; }

    float GetRf() const noexcept { return m_components[0]; }
    float GetGf() const noexcept { return m_components[1]; }
    float GetBf() const noexcept { return m_components[2]; }

    template<size_t sz = vector_size, typename = std::enable_if_t<sz>= 4, void>>
    float GetAf() const noexcept { return m_components[3]; }

    float GetNormRf() const noexcept { return GetNormColorComponent(GetRf()); }
    float GetNormGf() const noexcept { return GetNormColorComponent(GetGf()); }
    float GetNormBf() const noexcept { return GetNormColorComponent(GetBf()); }

    template<size_t sz = vector_size, typename = std::enable_if_t<sz >= 4, void>>
    float GetNormAf() const noexcept { return GetNormColorComponent(GetAf()); }

    uint8_t GetRu() const noexcept { return GetUintColorComponent(GetNormRf()); }
    uint8_t GetGu() const noexcept { return GetUintColorComponent(GetNormGf()); }
    uint8_t GetBu() const noexcept { return GetUintColorComponent(GetNormBf()); }

    template<size_t sz = vector_size, typename = std::enable_if_t<sz >= 4, void>>
    uint8_t GetAu() const noexcept { return GetUintColorComponent(GetNormAf()); }

    void Set(Data::Index component_index, float value)
    {
        META_CHECK_ARG_LESS(component_index, Size);
        META_CHECK_ARG_RANGE(value, s_float_range.first, s_float_range.second);
        m_components[component_index] = value;
    }
    void SetR(float r) { Set(0, r); }
    void SetG(float g) { Set(1, g); }
    void SetB(float b) { Set(2, b); }

    template<size_t sz = vector_size, typename = std::enable_if_t<sz >= 4, void>>
    void SetA(float a) { Set(3, a); }

    explicit operator std::string() const noexcept
    {
        if constexpr (vector_size == 3)
            return fmt::format("C(r:{:d}, g:{:d}, b:{:d})", GetRu(), GetGu(), GetBu());
        else
            return fmt::format("C(r:{:d}, g:{:d}, b:{:d}, a:{:a})", GetRu(), GetGu(), GetBu(), GetAu());
    }

private:
    inline float GetNormColorComponent(float component) const noexcept
    {
        return std::max(s_float_range.first, std::min(s_float_range.second, component));
    }

    inline uint8_t GetUintColorComponent(float component) const noexcept
    {
        return static_cast<uint8_t>(std::round(component * static_cast<float>(s_uint_component_max)));
    }

    VectorType m_components;

    static constexpr std::pair<float, float> s_float_range{ 0.F, 1.F };
    static constexpr uint8_t s_uint_component_max = std::numeric_limits<uint8_t>::max();
};

using Color3f = ColorF<3>;
using Color4f = ColorF<4>;

} // namespace Methane::Graphics
