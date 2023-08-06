/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/TypeFormatters.hpp
Methane Graphics RHI Type formatters for use with fmt::format(...)

******************************************************************************/

#pragma once

#include "ResourceView.h"

template<>
struct fmt::formatter<Methane::Graphics::Rhi::SubResource::Index>
{
    template<typename FormatContext>
    auto format(const Methane::Graphics::Rhi::SubResource::Index& index, FormatContext& ctx) const
    { return format_to(ctx.out(), "{}", static_cast<std::string>(index)); }

    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const
    { return ctx.end(); }
};

template<>
struct fmt::formatter<Methane::Graphics::Rhi::SubResource::Count>
{
    template<typename FormatContext>
    auto format(const Methane::Graphics::Rhi::SubResource::Count& count, FormatContext& ctx) const
    { return format_to(ctx.out(), "{}", static_cast<std::string>(count)); }

    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const
    { return ctx.end(); }
};