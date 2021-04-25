/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy

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

FILE: Methane/Data/BitMaskHelpers.hpp
Bit-mask helpers.

******************************************************************************/

#include <magic_enum.hpp>

namespace Methane::Data
{

template<typename EnumType>
static std::string GetBitMaskFlagNames(EnumType mask_value, EnumType none_value = EnumType(0), EnumType all_value = EnumType(~0))
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;

    if (mask_value == none_value)
        return "None";

    if (mask_value == all_value)
        return "All";

    std::stringstream ss;
    for (const auto& flag_value : magic_enum::flags::enum_values<EnumType>())
        if (!magic_enum::flags::enum_contains(mask_value & flag_value))
            ss << magic_enum::flags::enum_name(flag_value) << "|";

    std::string mask_name = ss.str();
    if (!mask_name.empty())
        mask_name.erase(mask_name.length() - 1); // erase trailing | symbol

    return mask_name;
}

} // namespace Methane::Data
