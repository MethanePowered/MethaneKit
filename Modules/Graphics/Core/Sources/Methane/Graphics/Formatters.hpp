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

FILE: Methane/Graphics/Formatters.hpp
Methane graphics object formatters for use with fmt::format(...)

******************************************************************************/

#pragma once

#include <Methane/Graphics/Resource.h>
#include <Methane/Graphics/Program.h>

#include <fmt/format.h>

template<>
struct fmt::formatter<Methane::Graphics::Resource::SubResource::Index>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.end(); }

    template<typename FormatContext>
    auto format(const Methane::Graphics::Resource::SubResource::Index& index, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{}", static_cast<std::string>(index));
    }
};

template<>
struct fmt::formatter<Methane::Graphics::Resource::SubResource::Count>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.end(); }

    template<typename FormatContext>
    auto format(const Methane::Graphics::Resource::SubResource::Count& count, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{}", static_cast<std::string>(count));
    }
};

template<>
struct fmt::formatter<Methane::Graphics::Program::Argument>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.end(); }

    template<typename FormatContext>
    auto format(const Methane::Graphics::Program::Argument& program_argument, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{}", static_cast<std::string>(program_argument));
    }
};