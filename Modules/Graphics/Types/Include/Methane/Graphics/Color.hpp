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

#include "Types.h"

#include <Methane/Checks.hpp>

#include <fmt/format.h>
#include <cstdint>

namespace Methane::Graphics
{

template<size_t vector_size, typename VectorType = std::enable_if_t<3 <= vector_size && vector_size <= 4, cml::vector<float, cml::fixed<vector_size>>>>
class ColorNf : public VectorType
{
public:
    using VectorType::VectorType;
    using VectorType::operator=;

    float GetRf() const noexcept { return (*this)[0]; }
    float GetGf() const noexcept { return (*this)[1]; }
    float GetBf() const noexcept { return (*this)[2]; }

    template<size_t sz = vector_size, typename = std::enable_if_t<sz >= 4, void>>
    float GetAf() const noexcept { return (*this)[3]; }

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

    void SetR(float r) { META_CHECK_ARG_RANGE(r, s_float_range.first, s_float_range.second); (*this)[0] = r; }
    void SetG(float g) { META_CHECK_ARG_RANGE(g, s_float_range.first, s_float_range.second); (*this)[1] = g; }
    void SetB(float b) { META_CHECK_ARG_RANGE(b, s_float_range.first, s_float_range.second); (*this)[2] = b; }

    template<size_t sz = vector_size, typename = std::enable_if_t<sz >= 4, void>>
    void SetA(float a) { META_CHECK_ARG_RANGE(a, s_float_range.first, s_float_range.second); (*this)[3] = a; }

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

    static constexpr std::pair<float, float> s_float_range{ 0.F, 1.F };
    static constexpr uint8_t s_uint_component_max = std::numeric_limits<uint8_t>::max();
};

using Color3f = ColorNf<3>;
using Color4f = ColorNf<4>;

} // namespace Methane::Graphics
