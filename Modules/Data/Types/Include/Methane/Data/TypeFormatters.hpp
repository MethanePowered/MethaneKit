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

FILE: Methane/Data/TypeFormatters.hpp
Methane data type formatters for use with fmt::format(...)

******************************************************************************/

#pragma once

#include "Vector.hpp"
#include "Point.hpp"
#include "Rect.hpp"

#include <fmt/format.h>

template<>
struct fmt::formatter<hlslpp::float2>
{
    template<typename FormatContext>
    [[nodiscard]] auto format(const hlslpp::float2& v, FormatContext& ctx) const
    { return format_to(ctx.out(), "V({}, {})", v[0], v[1]); }

    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const
    { return ctx.end(); }
};

template<>
struct fmt::formatter<hlslpp::float3>
{
    template<typename FormatContext>
    [[nodiscard]] auto format(const hlslpp::float3& v, FormatContext& ctx) const
    { return format_to(ctx.out(), "V({}, {}, {})", v[0], v[1], v[2]); }

    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const
    { return ctx.end(); }
};

template<>
struct fmt::formatter<hlslpp::float4>
{
    template<typename FormatContext>
    [[nodiscard]] auto format(const hlslpp::float4& v, FormatContext& ctx) const
    { return format_to(ctx.out(), "V({}, {}, {}, {})", v[0], v[1], v[2], v[3]); }

    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const
    { return ctx.end(); }
};

template<typename T, size_t size>
struct fmt::formatter<Methane::Data::RawVector<T, size>>
{
    template<typename FormatContext>
    [[nodiscard]] auto format(const Methane::Data::RawVector<T, size>& v, FormatContext& ctx) const
    { return format_to(ctx.out(), "{}", static_cast<std::string>(v)); }

    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const
    { return ctx.end(); }
};

template<typename T, size_t vector_size>
struct fmt::formatter<Methane::Data::Point<T, vector_size>>
{
    template<typename FormatContext>
    [[nodiscard]] auto format(const Methane::Data::Point<T, vector_size>& point, FormatContext& ctx) const
    { return format_to(ctx.out(), "{}", static_cast<std::string>(point)); }

    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const
    { return ctx.end(); }
};

template<typename D>
struct fmt::formatter<Methane::Data::RectSize<D>>
{
    template<typename FormatContext>
    [[nodiscard]] auto format(const Methane::Data::RectSize<D>& rect_size, FormatContext& ctx) const
    { return format_to(ctx.out(), "{}", static_cast<std::string>(rect_size)); }

    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const
    { return ctx.end(); }
};

template<typename T, typename D>
struct fmt::formatter<Methane::Data::Rect<T, D>>
{
    template<typename FormatContext>
    [[nodiscard]] auto format(const Methane::Data::Rect<T, D>& rect, FormatContext& ctx) const
    { return format_to(ctx.out(), "{}", static_cast<std::string>(rect)); }

    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const
    { return ctx.end(); }
};