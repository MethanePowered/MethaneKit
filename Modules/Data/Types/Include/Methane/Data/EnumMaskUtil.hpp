/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: Methane/Data/EnumMaskUtil.hpp
EnumMask utility functions based on magic-enum.

******************************************************************************/

#pragma once

#include "EnumMask.hpp"

#include <Methane/Instrumentation.h>

#include <magic_enum.hpp>
#include <vector>
#include <array>
#include <string>
#include <string_view>
#include <sstream>

namespace Methane::Data
{

namespace Impl
{

template<typename E, typename M, std::size_t N, std::size_t... I>
constexpr std::array<typename EnumMask<E, M>::Bit, N> GetEnumMaskBitsArray(M, const std::array<E, N>& enums, std::index_sequence<I...>)
{
    return { { typename EnumMask<E, M>::Bit(enums[I])... } };
}

template<size_t Start, size_t End, typename T, size_t N, typename F>
constexpr void ConstexprForEach(const std::array<T, N>& values, F&& functor)
{
    if constexpr (Start < End)
    {
        functor(values[Start]);
        ConstexprForEach<Start + 1, End>(values, functor);
    }
}

template<size_t Start, size_t End, typename F>
constexpr void ConstexprForEachIndex(F&& functor)
{
    if constexpr (Start < End)
    {
        functor(std::integral_constant<size_t, Start>());
        ConstexprForEachIndex<Start + 1, End>(functor);
    }
}

} // namespace Impl

template <typename E, typename M, size_t N = magic_enum::enum_count<E>()>
constexpr auto GetEnumMaskBitsArray()
{
    return Impl::GetEnumMaskBitsArray(M{}, magic_enum::enum_values<E>(), std::make_index_sequence<N>{});
}

template <typename EnumMaskType>
constexpr auto GetEnumMaskBitsArray()
{
    return GetEnumMaskBitsArray<typename EnumMaskType::EnumType, typename EnumMaskType::MaskType>();
}

template<typename T, size_t N, typename F>
constexpr void ConstexprForEach(const std::array<T, N>& values, F&& functor)
{
    Impl::ConstexprForEach<0, N, T, N, F>(values, std::move(functor));
}

template<typename E, typename M, typename F>
constexpr void ForEachBitInEnumMask(EnumMask<E, M> mask, F&& functor)
{
    static constexpr auto s_enum_bits = GetEnumMaskBitsArray<E, M>();
    Impl::ConstexprForEachIndex<0, s_enum_bits.size()>(
        [mask, functor](auto index) constexpr
        {
            constexpr auto bit = std::get<index>(s_enum_bits);
            // It would be nice to use 'if constexpr' here in some cases, but 'mask' parameter can not be treated as constexpr
            if (mask.HasAnyBit(bit))
            {
                functor(bit.GetEnum());
            }
        });
}

template<typename E, typename M>
std::vector<E> GetEnumMaskBits(EnumMask<E, M> mask)
{
    META_FUNCTION_TASK();
    std::vector<E> bits;
    ForEachBitInEnumMask(mask, [&bits](E bit)
    {
        bits.push_back(bit);
    });
    return bits;
}

template<typename E, typename M>
std::vector<std::string> GetEnumMaskBitNames(EnumMask<E, M> mask)
{
    META_FUNCTION_TASK();
    std::vector<std::string> bit_names;
    ForEachBitInEnumMask(mask, [&bit_names](E bit)
    {
        bit_names.emplace_back(magic_enum::enum_name(bit));
    });
    return bit_names;
}

template<typename E, typename M>
std::string GetEnumMaskName(EnumMask<E, M> mask, std::string_view separator = "|")
{
    META_FUNCTION_TASK();
    if (!mask)
        return "";

    std::stringstream ss;
    ss << "(";
    ForEachBitInEnumMask(mask, [&ss, separator](E bit)
    {
        ss << magic_enum::enum_name(bit) << separator;
    });

    if (separator.length() <= 1)
    {
        ss.seekp(-static_cast<int>(separator.length()), std::ios_base::end);
        ss << ')';
        return ss.str();
    }

    std::string s = ss.str();
    return s.erase(s.length() - separator.length()).append(")");
}

} // namespace Methane::Data
