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

#include "Point.hpp"
#include "Rect.hpp"

#include <hlsl++.h>
#include <cml/vector.h>
#include <fmt/format.h>

template<>
struct fmt::formatter<hlslpp::float2>
{
    template<typename FormatContext>
    [[nodiscard]] auto format(const hlslpp::float2& v, FormatContext& ctx) { return format_to(ctx.out(), "V({}, {})", v.x, v.y); }
    constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }
};

template<>
struct fmt::formatter<hlslpp::float3>
{
    template<typename FormatContext>
    [[nodiscard]] auto format(const hlslpp::float3& v, FormatContext& ctx) { return format_to(ctx.out(), "V({}, {}, {})", v.x, v.y, v.z); }
    constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }
};

template<>
struct fmt::formatter<hlslpp::float4>
{
    template<typename FormatContext>
    [[nodiscard]] auto format(const hlslpp::float4& v, FormatContext& ctx) { return format_to(ctx.out(), "V({}, {}, {}, {})", v.x, v.y, v.z, v.w); }
    constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }
};

template<typename T>
struct fmt::formatter<cml::vector<T, cml::fixed<2>>>
{
    template<typename FormatContext>
    [[nodiscard]] auto format(const cml::vector<T, cml::fixed<2>>& v, FormatContext& ctx) { return format_to(ctx.out(), "V({}, {})", v[0], v[1]); }
    constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }
};

template<typename T>
struct fmt::formatter<cml::vector<T, cml::fixed<3>>>
{
    template<typename FormatContext>
    [[nodiscard]] auto format(const cml::vector<T, cml::fixed<3>>& v, FormatContext& ctx) { return format_to(ctx.out(), "V({}, {}, {})", v[0], v[1], v[2]); }
    constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }
};

template<typename T>
struct fmt::formatter<cml::vector<T, cml::fixed<4>>>
{
    template<typename FormatContext>
    [[nodiscard]] auto format(const cml::vector<T, cml::fixed<4>>& v, FormatContext& ctx) { return format_to(ctx.out(), "V({}, {}, {}, {})", v[0], v[1], v[3], v[3]); }
    constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }
};

template<typename T, size_t vector_size>
struct fmt::formatter<Methane::Data::PointT<T, vector_size>>
{
    template<typename FormatContext>
    auto format(const Methane::Data::PointT<T, vector_size>& point, FormatContext& ctx) { return format_to(ctx.out(), "{}", static_cast<std::string>(point)); }
    constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }
};

template<typename D>
struct fmt::formatter<Methane::Data::RectSize<D>>
{
    template<typename FormatContext>
    auto format(const Methane::Data::RectSize<D>& rect_size, FormatContext& ctx) { return format_to(ctx.out(), "{}", static_cast<std::string>(rect_size)); }
    constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }
};

template<typename T, typename D>
struct fmt::formatter<Methane::Data::Rect<T, D>>
{
    template<typename FormatContext>
    auto format(const Methane::Data::Rect<T, D>& rect, FormatContext& ctx) { return format_to(ctx.out(), "{}", static_cast<std::string>(rect)); }
    constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }
};