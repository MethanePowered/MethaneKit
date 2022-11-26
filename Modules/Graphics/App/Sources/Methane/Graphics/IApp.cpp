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

FILE: Methane/Graphics/App.h
Interface of the graphics application base template class defined in App.hpp

******************************************************************************/

#include <Methane/Graphics/IApp.h>

namespace Methane::Graphics
{

AppSettings& AppSettings::SetScreenPassAccess(Rhi::RenderPassAccess::Mask new_screen_pass_access) noexcept
{
    META_FUNCTION_TASK();
    screen_pass_access = new_screen_pass_access;
    return *this;
}

AppSettings& AppSettings::SetAnimationsEnabled(bool new_animations_enabled) noexcept
{
    META_FUNCTION_TASK();
    animations_enabled = new_animations_enabled;
    return *this;
}

AppSettings& AppSettings::SetShowHudInWindowTitle(bool new_show_hud_in_window_title) noexcept
{
    META_FUNCTION_TASK();
    show_hud_in_window_title = new_show_hud_in_window_title;
    return *this;
}

AppSettings& AppSettings::SetDefaultDeviceIndex(int32_t new_default_device_index) noexcept
{
    META_FUNCTION_TASK();
    default_device_index = new_default_device_index;
    return *this;
}

AppSettings& AppSettings::SetDeviceCapabilities(Rhi::DeviceCaps&& new_device_capabilities) noexcept
{
    META_FUNCTION_TASK();
    device_capabilities = std::move(new_device_capabilities);
    return *this;
}

} // namespace Methane::Graphics
