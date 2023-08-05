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

template<typename D>
struct fmt::formatter<Methane::Graphics::VolumeSize<D>>
{
    template<typename FormatContext>
    [[nodiscard]] auto format(const Methane::Graphics::VolumeSize<D>& vol_size, FormatContext& ctx) const
    { return format_to(ctx.out(), "{}", static_cast<std::string>(vol_size)); }

    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const
    { return ctx.end(); }
};

template<typename T, typename D>
struct fmt::formatter<Methane::Graphics::Volume<T, D>>
{
    template<typename FormatContext>
    [[nodiscard]] auto format(const Methane::Graphics::Volume<T, D>& vol, FormatContext& ctx) const
    { return format_to(ctx.out(), "{}", static_cast<std::string>(vol)); }

    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const
    { return ctx.end(); }
};

template<size_t color_size>
struct fmt::formatter<Methane::Graphics::ColorF<color_size>>
{
    template<typename FormatContext>
    [[nodiscard]] auto format(const Methane::Graphics::ColorF<color_size>& color, FormatContext& ctx) const
    { return format_to(ctx.out(), "{}", static_cast<std::string>(color)); }

    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const
    { return ctx.end(); }
};
