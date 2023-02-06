/******************************************************************************

Copyright 2019-2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/ISystem.h
Methane system interface to query graphics devices.

******************************************************************************/

#pragma once

#include "IDevice.h"

#include <functional>

namespace Methane::Platform
{
struct AppEnvironment;
}

namespace Methane::Graphics::Rhi
{

enum class NativeApi
{
    Undefined,
    Metal,
    DirectX,
    Vulkan
};

struct ISystem
{
    [[nodiscard]] static NativeApi GetNativeApi() noexcept;
    [[nodiscard]] static ISystem& Get();

    virtual void CheckForChanges() = 0;
    virtual const Ptrs<IDevice>& UpdateGpuDevices(const DeviceCaps& required_device_caps = {}) = 0;
    virtual const Ptrs<IDevice>& UpdateGpuDevices(const Platform::AppEnvironment& app_env,
                                                  const DeviceCaps& required_device_caps = {}) = 0;
    [[nodiscard]] virtual const Ptrs<IDevice>& GetGpuDevices() const noexcept = 0;
    [[nodiscard]] virtual Ptr<IDevice>         GetNextGpuDevice(const IDevice& device) const noexcept = 0;
    [[nodiscard]] virtual Ptr<IDevice>         GetSoftwareGpuDevice() const noexcept = 0;
    [[nodiscard]] virtual const DeviceCaps&    GetDeviceCapabilities() const noexcept = 0;
    [[nodiscard]] virtual std::string          ToString() const = 0;
    
    virtual ~ISystem() = default;
};

} // namespace Methane::Graphics::Rhi
