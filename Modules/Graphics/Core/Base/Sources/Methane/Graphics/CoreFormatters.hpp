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

FILE: Methane/Graphics/CoreFormatters.hpp
Methane Graphics Core objects formatters for use with fmt::format(...)

******************************************************************************/

#pragma once

#include <Methane/Graphics/IResource.h>
#include <Methane/Graphics/IProgram.h>

#include <fmt/format.h>

template<>
struct fmt::formatter<Methane::Graphics::IResource::SubResource::Index>
{
    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }

    template<typename FormatContext>
    auto format(const Methane::Graphics::IResource::SubResource::Index& index, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{}", static_cast<std::string>(index));
    }
};

template<>
struct fmt::formatter<Methane::Graphics::IResource::SubResource::Count>
{
    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }

    template<typename FormatContext>
    auto format(const Methane::Graphics::IResource::SubResource::Count& count, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{}", static_cast<std::string>(count));
    }
};

template<>
struct fmt::formatter<Methane::Graphics::IProgram::Argument>
{
    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }

    template<typename FormatContext>
    auto format(const Methane::Graphics::IProgram::Argument& program_argument, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{}", static_cast<std::string>(program_argument));
    }
};