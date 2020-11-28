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

#include <fmt/format.h>

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