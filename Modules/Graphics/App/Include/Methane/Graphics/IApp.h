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

#pragma once

#include <Methane/Graphics/RHI/IRenderPass.h>
#include <Methane/Graphics/RHI/IDevice.h>

#include <stdint.h>

namespace Methane::Graphics
{

struct AppSettings
{
    Rhi::RenderPassAccess screen_pass_access       = Rhi::RenderPassAccess::None;
    bool                  animations_enabled       = true;
    bool                  show_hud_in_window_title = true;
    int32_t               default_device_index     = 0;    // 0 - default h/w GPU, 1 - second h/w GPU, -1 - emulated WARP device
    Rhi::DeviceCaps       device_capabilities;

    AppSettings& SetScreenPassAccess(Rhi::RenderPassAccess new_screen_pass_access) noexcept;
    AppSettings& SetAnimationsEnabled(bool new_animations_enabled) noexcept;
    AppSettings& SetShowHudInWindowTitle(bool new_show_hud_in_window_title) noexcept;
    AppSettings& SetDefaultDeviceIndex(int32_t new_default_device_index) noexcept;
    AppSettings& SetDeviceCapabilities(Rhi::DeviceCaps&& new_device_capabilities) noexcept;
};

struct IApp
{
    using Settings = AppSettings;

    virtual const Settings& GetGraphicsAppSettings() const noexcept = 0;
    virtual bool SetAnimationsEnabled(bool animations_enabled) = 0;

    virtual ~IApp() = default;
};

} // namespace Methane::Graphics
