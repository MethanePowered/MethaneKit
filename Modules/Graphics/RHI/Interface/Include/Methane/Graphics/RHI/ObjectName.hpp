/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/ObjectName.hpp
Methane graphics RHI object naming utility function

******************************************************************************/

#pragma once

#include <fmt/format.h>

namespace Methane::Graphics::Rhi
{

template<typename ObjectType, typename FormatType, typename... ArgTypes>
bool SetObjectName(ObjectType& object, FormatType&& format, ArgTypes&& ... args)
{
    const std::string formatted_name = fmt::format(std::forward<FormatType>(format), std::forward<ArgTypes>(args)...);
    return object.SetName(formatted_name);
}

} // namespace Methane::Graphics::Rhi
