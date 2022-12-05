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

FILE: Methane/Graphics/RHI/Device.h
Methane System and Device PIMP wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include <Methane/Graphics/RHI/IDevice.h>

namespace Methane::Graphics::Rhi
{

class Device
{
    friend class System;

public:
    using FeatureMask  = DeviceFeatureMask;
    using Feature      = DeviceFeature;
    using Capabilities = DeviceCaps;

    const std::string&  GetAdapterName() const noexcept;
    bool                IsSoftwareAdapter() const noexcept;
    const Capabilities& GetCapabilities() const noexcept;
    std::string         ToString() const;

    IDevice& GetInterface() const noexcept;

private:
    Device(const Ptr<IDevice>& device_ptr);

    class Impl;

    UniquePtr<Impl> m_impl_ptr;
};

using Devices = std::vector<Device>;

class System
{
public:
    [[nodiscard]] static System& Get();

    void CheckForChanges() const;
    [[nodiscard]] const Devices&    UpdateGpuDevices(const DeviceCaps& required_device_caps = {}) const;
    [[nodiscard]] const Devices&    UpdateGpuDevices(const Platform::AppEnvironment& app_env,
                                                     const DeviceCaps& required_device_caps = {}) const;
    [[nodiscard]] const Devices&    GetGpuDevices() const noexcept;
    [[nodiscard]] Device            GetNextGpuDevice(const Device& device) const noexcept;
    [[nodiscard]] Device            GetSoftwareGpuDevice() const noexcept;
    [[nodiscard]] const DeviceCaps& GetDeviceCapabilities() const noexcept;
    [[nodiscard]] std::string       ToString() const;

    ISystem& GetInterface() const noexcept;

private:
    System(const Ptr<ISystem>& system_ptr);

    const Devices& UpdateDevices(const Ptrs<Rhi::IDevice>& devices) const;

    class Impl;

    UniquePtr<Impl> m_impl_ptr;
    mutable Devices m_devices;
};

} // namespace Methane::Graphics::Rhi
