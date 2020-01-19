/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Platform/Input/HelpProvider.h
Abstract input help provider.

******************************************************************************/

#pragma once

#include <vector>
#include <string>

namespace Methane::Platform::Input
{

struct IHelpProvider
{
    using KeyDescription = std::pair<std::string /* keys */, std::string /* description */>; // keys are empty for headers
    using HelpLines      = std::vector<KeyDescription>;

    virtual HelpLines GetHelp() const = 0;

    virtual ~IHelpProvider() = default;
};

} // namespace Methane::Platform::Input
