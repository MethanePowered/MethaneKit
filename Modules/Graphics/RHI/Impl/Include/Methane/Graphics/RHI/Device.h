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
Methane System and Device PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include "Pimpl.h"

#include <Methane/Graphics/RHI/IDevice.h>
#include <Methane/Data/Transmitter.hpp>

namespace Methane::Graphics::Rhi
{

class Device
    : public Data::Transmitter<IDeviceCallback>
{
    friend class System;

public:
    using FeatureMask  = DeviceFeatureMask;
    using Feature      = DeviceFeature;
    using Capabilities = DeviceCaps;

    META_PIMPL_METHODS_DECLARE(Device);

    Device(const Ptr<IDevice>& interface_ptr);
    Device(IDevice& interface_ref);

    IDevice& GetInterface() const META_PIMPL_NOEXCEPT;
    Ptr<IDevice> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    bool SetName(std::string_view name) const;
    std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // IDevice interface methods
    const std::string&  GetAdapterName() const META_PIMPL_NOEXCEPT;
    bool                IsSoftwareAdapter() const META_PIMPL_NOEXCEPT;
    const Capabilities& GetCapabilities() const META_PIMPL_NOEXCEPT;
    std::string         ToString() const;

private:
    class Impl;

    Device(ImplPtr<Impl>&& impl_ptr);

    ImplPtr<Impl> m_impl_ptr;
};

using Devices = std::vector<Device>;

class System
{
public:
    [[nodiscard]] static System& Get();

    META_PIMPL_METHODS_DECLARE(System);

    System(const Ptr<ISystem>& interface_ptr);

    ISystem& GetInterface() const META_PIMPL_NOEXCEPT;

    // ISystem interface methods
    void CheckForChanges() const;
    [[nodiscard]] const Devices&    UpdateGpuDevices(const DeviceCaps& required_device_caps = {}) const;
    [[nodiscard]] const Devices&    UpdateGpuDevices(const Platform::AppEnvironment& app_env,
                                                     const DeviceCaps& required_device_caps = {}) const;
    [[nodiscard]] const Devices&    GetGpuDevices() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] Device            GetNextGpuDevice(const Device& device) const META_PIMPL_NOEXCEPT;
    [[nodiscard]] Device            GetSoftwareGpuDevice() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] const DeviceCaps& GetDeviceCapabilities() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] std::string       ToString() const;

private:
    const Devices& UpdateDevices(const Ptrs<Rhi::IDevice>& devices) const;

    class Impl;

    ImplPtr<Impl> m_impl_ptr;
    mutable Devices m_devices;
};

} // namespace Methane::Graphics::Rhi
