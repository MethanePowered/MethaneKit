/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/TypeFormatters.hpp
Methane Graphics Type formatters for use with fmt::format(...)

******************************************************************************/

#pragma once

#include "Volume.hpp"
#include "Color.hpp"

#include <Methane/Data/TypeFormatters.hpp>

#include <cml/vector.h>
#include <fmt/format.h>

template<typename T>
struct fmt::formatter<cml::vector<T, cml::fixed<2>>>
{
    template<typename FormatContext>
    auto format(const cml::vector<T, cml::fixed<2>>& v, FormatContext& ctx) { return format_to(ctx.out(), "V({}, {})", v[0], v[1]); }
    constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }
};

template<typename T>
struct fmt::formatter<cml::vector<T, cml::fixed<3>>>
{
    template<typename FormatContext>
    auto format(const cml::vector<T, cml::fixed<3>>& v, FormatContext& ctx) { return format_to(ctx.out(), "V({}, {}, {})", v[0], v[1], v[2]); }
    constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }
};

template<typename T>
struct fmt::formatter<cml::vector<T, cml::fixed<4>>>
{
    template<typename FormatContext>
    auto format(const cml::vector<T, cml::fixed<4>>& v, FormatContext& ctx) { return format_to(ctx.out(), "V({}, {}, {}, {})", v[0], v[1], v[3], v[3]); }
    constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }
};

template<typename D>
struct fmt::formatter<Methane::Graphics::VolumeSize<D>>
{
    template<typename FormatContext>
    auto format(const Methane::Graphics::VolumeSize<D>& vol_size, FormatContext& ctx) { return format_to(ctx.out(), "{}", static_cast<std::string>(vol_size)); }
    constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }
};

template<typename T, typename D>
struct fmt::formatter<Methane::Graphics::Volume<T, D>>
{
    template<typename FormatContext>
    auto format(const Methane::Graphics::Volume<T, D>& vol, FormatContext& ctx) { return format_to(ctx.out(), "{}", static_cast<std::string>(vol)); }
    constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }
};

template<size_t color_size>
struct fmt::formatter<Methane::Graphics::ColorF<color_size>>
{
    template<typename FormatContext>
    auto format(const Methane::Graphics::ColorF<color_size>& color, FormatContext& ctx) { return format_to(ctx.out(), "{}", static_cast<std::string>(color)); }
    constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }
};
