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
#include <string>
#include <string_view>
#include <sstream>

namespace Methane::Data
{

template<typename E, typename M>
std::vector<E> GetEnumMaskBits(EnumMask<E, M> mask)
{
    META_FUNCTION_TASK();
    std::vector<E> bits;
    for(E bit : magic_enum::enum_values<E>())
    {
        if (mask.HasAnyBit(bit))
            bits.push_back(bit);
    }
    return bits;
}

template<typename E, typename M>
std::vector<std::string> GetEnumBitNames(EnumMask<E, M> mask)
{
    META_FUNCTION_TASK();
    std::vector<std::string> bit_names;
    for(E bit : magic_enum::enum_values<E>())
    {
        if (mask.HasAnyBit(bit))
            bit_names.emplace_back(magic_enum::enum_name(bit));
    }
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
    for(E bit : magic_enum::enum_values<E>())
    {
        if (mask.HasAnyBit(bit))
            ss << magic_enum::enum_name(bit) << separator;
    }
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
