/******************************************************************************

Copyright 2020-2022 Evgeny Gorodetskiy

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

FILE: Methane/Tutorials/AppSettings.hpp
Common application settings for Methane samples and tutorials.

******************************************************************************/

#pragma once

#include <Methane/Graphics/CombinedAppSettings.h>
#include <Methane/UserInterface/IApp.h>
#include <Methane/Data/EnumMask.hpp>

namespace Methane::Tutorials
{

namespace AppOptions
{
    enum class Bit
    {
        DepthBuffer,
        ClearDepth,
        ClearColor,
        Animations,
        FullScreen,
        VSync,
        HudVisible
    };

    using Mask = Data::EnumMask<Bit>;

    Mask GetDefaultWithColorOnly() noexcept;
    Mask GetDefaultWithColorDepth() noexcept;
    Mask GetDefaultWithColorDepthAndAnim() noexcept;
    Mask GetDefaultWithColorOnlyAndAnim() noexcept;
};

[[nodiscard]] Graphics::CombinedAppSettings GetGraphicsTutorialAppSettings(const std::string& app_name, AppOptions::Mask app_options);
[[nodiscard]] UserInterface::AppSettings    GetUserInterfaceTutorialAppSettings(AppOptions::Mask app_options);

} // namespace Methane::Graphics
