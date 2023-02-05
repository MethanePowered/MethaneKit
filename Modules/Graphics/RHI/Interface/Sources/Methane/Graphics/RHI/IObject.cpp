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

FILE: Methane/Graphics/RHI/IObject.cpp
Methane object interface: represents any named object.

******************************************************************************/

#include <Methane/Graphics/RHI/IObject.h>

#include <Methane/Instrumentation.h>

#include <fmt/format.h>

namespace Methane::Graphics::Rhi
{

NameConflictException::NameConflictException(std::string_view name)
    : std::invalid_argument(fmt::format("Can not add graphics object with name {} to the registry because another object with the same name is already registered.", name))
{ }

} // namespace Methane::Graphics::Rhi
