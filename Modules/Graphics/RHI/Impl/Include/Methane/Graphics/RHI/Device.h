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
Methane Device PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include "Pimpl.h"

#include <Methane/Graphics/RHI/IDevice.h>

namespace Methane::Graphics::META_GFX_NAME
{
class Device;
}

namespace Methane::Graphics::Rhi
{

class RenderContext;

class Device // NOSONAR - constructors and assignment operators are required to use forward declared Impl and Ptr<Impl> in header
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
    [[nodiscard]] META_RHI_API RenderContext       CreateRenderContext(const Platform::AppEnvironment& env, tf::Executor& parallel_executor, const RenderContextSettings& settings) const;
    [[nodiscard]] META_RHI_API const std::string&  GetAdapterName() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API bool                IsSoftwareAdapter() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API const Capabilities& GetCapabilities() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API std::string         ToString() const;

    // Data::IEmitter<IDeviceCallback> interface methods
    META_RHI_API void Connect(Data::Receiver<IDeviceCallback>& receiver) const;
    META_RHI_API void Disconnect(Data::Receiver<IDeviceCallback>& receiver) const;

private:
    using Impl = Methane::Graphics::META_GFX_NAME::Device;

    Ptr<Impl> m_impl_ptr;
};

using Devices = std::vector<Device>;

} // namespace Methane::Graphics::Rhi

#ifdef META_RHI_PIMPL_INLINE

#include <Methane/Graphics/RHI/Device.cpp>

#endif // META_RHI_PIMPL_INLINE
