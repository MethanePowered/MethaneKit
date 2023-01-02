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

namespace Methane::Graphics::META_GFX_NAME
{
class Device;
class System;
}

namespace Methane::Graphics::Rhi
{

class Device
{
    friend class System;

public:
    using FeatureMask  = DeviceFeatureMask;
    using Feature      = DeviceFeature;
    using Capabilities = DeviceCaps;

    META_PIMPL_METHODS_DECLARE(Device);
    META_PIMPL_METHODS_COMPARE_DECLARE(Device);

    META_RHI_API explicit Device(const Ptr<IDevice>& interface_ptr);
    META_RHI_API explicit Device(IDevice& interface_ref);

    META_RHI_API bool IsInitialized() const META_PIMPL_NOEXCEPT { return true; }
    META_RHI_API IDevice& GetInterface() const META_PIMPL_NOEXCEPT;
    META_RHI_API Ptr<IDevice> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    META_RHI_API bool SetName(std::string_view name) const;
    META_RHI_API std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // IDevice interface methods
    [[nodiscard]] META_RHI_API const std::string&  GetAdapterName() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API bool                IsSoftwareAdapter() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API const Capabilities& GetCapabilities() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API std::string         ToString() const;

    // Data::IEmitter<IDeviceCallback> interface methods
    META_RHI_API void Connect(Data::Receiver<IDeviceCallback>& receiver) const;
    META_RHI_API void Disconnect(Data::Receiver<IDeviceCallback>& receiver) const;

private:
    using Impl = Methane::Graphics::META_GFX_NAME::Device;

    META_RHI_API Device(Ptr<Impl>&& impl_ptr);

    Ptr<Impl> m_impl_ptr;
};

using Devices = std::vector<Device>;

class System
{
public:
    [[nodiscard]] META_RHI_API static NativeApi GetNativeApi() noexcept;
    [[nodiscard]] META_RHI_API static System& Get();

    META_PIMPL_METHODS_DECLARE(System);

    META_RHI_API explicit System(const Ptr<ISystem>& interface_ptr);

    META_RHI_API ISystem& GetInterface() const META_PIMPL_NOEXCEPT;

    // ISystem interface methods
    META_RHI_API void CheckForChanges() const;
    [[nodiscard]] META_RHI_API const Devices&    UpdateGpuDevices(const DeviceCaps& required_device_caps = {}) const;
    [[nodiscard]] META_RHI_API const Devices&    UpdateGpuDevices(const Platform::AppEnvironment& app_env,
                                                                  const DeviceCaps& required_device_caps = {}) const;
    [[nodiscard]] META_RHI_API const Devices&    GetGpuDevices() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API Device            GetNextGpuDevice(const Device& device) const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API Device            GetSoftwareGpuDevice() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API const DeviceCaps& GetDeviceCapabilities() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API std::string       ToString() const;

private:
    META_RHI_API const Devices& UpdateDevices(const Ptrs<Rhi::IDevice>& devices) const;

    using Impl = Methane::Graphics::META_GFX_NAME::System;

    Ptr<Impl> m_impl_ptr;
    mutable Devices m_devices;
};

} // namespace Methane::Graphics::Rhi

#ifdef META_RHI_PIMPL_INLINE

#include <Methane/Graphics/RHI/Device.cpp>

#endif // META_RHI_PIMPL_INLINE
