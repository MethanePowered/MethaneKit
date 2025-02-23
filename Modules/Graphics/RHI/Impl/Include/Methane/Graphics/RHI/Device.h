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

#include <Methane/Pimpl.h>

#include <Methane/Graphics/RHI/IDevice.h>

namespace Methane::Graphics::META_GFX_NAME
{
class Device;
}

namespace Methane::Graphics::Rhi
{

class RenderContext;
class ComputeContext;

class Device // NOSONAR - constructors and assignment operators are required to use forward declared Impl and Ptr<Impl> in header
{
    friend class System;

public:
    using FeatureMask  = DeviceFeatureMask;
    using Feature      = DeviceFeature;
    using Capabilities = DeviceCaps;

    META_PIMPL_METHODS_DECLARE(Device);
    META_PIMPL_METHODS_COMPARE_INLINE(Device);

    META_PIMPL_API explicit Device(const Ptr<IDevice>& interface_ptr);
    META_PIMPL_API explicit Device(IDevice& interface_ref);

    META_PIMPL_API bool IsInitialized() const META_PIMPL_NOEXCEPT { return true; }
    META_PIMPL_API IDevice& GetInterface() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API Ptr<IDevice> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    META_PIMPL_API bool SetName(std::string_view name) const;
    META_PIMPL_API std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // Data::IEmitter<IObjectCallback> interface methods
    META_PIMPL_API void Connect(Data::Receiver<IObjectCallback>& receiver) const;
    META_PIMPL_API void Disconnect(Data::Receiver<IObjectCallback>& receiver) const;

    // IDevice interface methods
    [[nodiscard]] META_PIMPL_API RenderContext       CreateRenderContext(const Platform::AppEnvironment& env, tf::Executor& parallel_executor, const RenderContextSettings& settings) const;
    [[nodiscard]] META_PIMPL_API ComputeContext      CreateComputeContext(tf::Executor& parallel_executor, const ComputeContextSettings& settings) const;
    [[nodiscard]] META_PIMPL_API const std::string&  GetAdapterName() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API bool                IsSoftwareAdapter() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API const Capabilities& GetCapabilities() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API std::string         ToString() const;

    // Data::IEmitter<IDeviceCallback> interface methods
    META_PIMPL_API void Connect(Data::Receiver<IDeviceCallback>& receiver) const;
    META_PIMPL_API void Disconnect(Data::Receiver<IDeviceCallback>& receiver) const;

private:
    using Impl = Methane::Graphics::META_GFX_NAME::Device;

    Ptr<Impl> m_impl_ptr;
};

using Devices = std::vector<Device>;

} // namespace Methane::Graphics::Rhi

#ifdef META_PIMPL_INLINE

#include <Methane/Graphics/RHI/Device.cpp>

#endif // META_PIMPL_INLINE
