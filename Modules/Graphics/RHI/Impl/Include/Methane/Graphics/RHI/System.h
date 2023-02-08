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

FILE: Methane/Graphics/RHI/System.h
Methane System PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include "Device.h"

#include <Methane/Pimpl.h>

#include <Methane/Graphics/RHI/ISystem.h>

namespace Methane::Graphics::META_GFX_NAME
{
class System;
}

namespace Methane::Graphics::Rhi
{

class System // NOSONAR - constructors and assignment operators are required to use forward declared Impl and Ptr<Impl> in header
{
public:
    [[nodiscard]] META_PIMPL_API static NativeApi GetNativeApi() noexcept;
    [[nodiscard]] META_PIMPL_API static System& Get();

    META_PIMPL_METHODS_DECLARE(System);

    META_PIMPL_API explicit System(const Ptr<ISystem>& interface_ptr);

    META_PIMPL_API ISystem& GetInterface() const META_PIMPL_NOEXCEPT;

    // ISystem interface methods
    META_PIMPL_API void CheckForChanges() const;
    META_PIMPL_API const Devices& UpdateGpuDevices(const DeviceCaps& required_device_caps = {}) const;
    META_PIMPL_API const Devices& UpdateGpuDevices(const Platform::AppEnvironment& app_env,
                                                   const DeviceCaps& required_device_caps = {}) const;
    [[nodiscard]] META_PIMPL_API const Devices&    GetGpuDevices() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API Device            GetNextGpuDevice(const Device& device) const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API Device            GetSoftwareGpuDevice() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API const DeviceCaps& GetDeviceCapabilities() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API std::string       ToString() const;

private:
    META_PIMPL_API const Devices& UpdateDevices(const Ptrs<Rhi::IDevice>& devices) const;

    using Impl = Methane::Graphics::META_GFX_NAME::System;

    Ptr<Impl> m_impl_ptr;
    mutable Devices m_devices;
};

} // namespace Methane::Graphics::Rhi

#ifdef META_PIMPL_INLINE

#include <Methane/Graphics/RHI/System.cpp>

#endif // META_PIMPL_INLINE
