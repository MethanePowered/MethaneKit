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

FILE: Methane/Graphics/Device.h
Methane device interface: describes system graphics device,
is used to create graphics context for rendering.

******************************************************************************/

#pragma once

#include "Object.h"

#include <Methane/Data/IEmitter.h>
#include <Methane/Memory.hpp>

#include <array>
#include <functional>

namespace Methane::Platform
{
struct AppEnvironment;
}

namespace Methane::Graphics
{

struct Device;

struct IDeviceCallback
{
    virtual void OnDeviceRemovalRequested(Device& device) = 0;
    virtual void OnDeviceRemoved(Device& device) = 0;

    virtual ~IDeviceCallback() = default;
};

struct Device
    : virtual Object // NOSONAR
    , virtual Data::IEmitter<IDeviceCallback> // NOSONAR
{
    enum class Features : uint32_t
    {
        Unknown              = 0U,
        BasicRendering       = 1U << 0U,
        AnisotropicFiltering = 1U << 2U,
        All                  = ~0U,
    };

    struct Capabilities
    {
        Features features            = Device::Features::All;
        bool     present_to_window   = true;
        uint32_t render_queues_count = 1U;
        uint32_t blit_queues_count   = 1U;

        Capabilities& SetFeatures(Features new_features) noexcept;
        Capabilities& SetPresentToWindow(bool new_present_to_window) noexcept;
        Capabilities& SetRenderQueuesCount(uint32_t new_render_queues_count) noexcept;
        Capabilities& SetBlitQueuesCount(uint32_t new_blit_queues_count) noexcept;
    };

    [[nodiscard]] virtual const std::string&  GetAdapterName() const noexcept = 0;
    [[nodiscard]] virtual bool                IsSoftwareAdapter() const noexcept = 0;
    [[nodiscard]] virtual const Capabilities& GetCapabilities() const noexcept = 0;
    [[nodiscard]] virtual std::string         ToString() const = 0;
};

struct System
{
    enum class GraphicsApi
    {
        Undefined,
        Metal,
        DirectX,
        Vulkan
    };

    static GraphicsApi GetGraphicsApi() noexcept;

    [[nodiscard]] static System& Get();

    virtual void CheckForChanges() = 0;
    [[nodiscard]] virtual const Ptrs<Device>&         UpdateGpuDevices(const Device::Capabilities& required_device_caps = {}) = 0;
    [[nodiscard]] virtual const Ptrs<Device>&         UpdateGpuDevices(const Platform::AppEnvironment& app_env, const Device::Capabilities& required_device_caps = {}) = 0;
    [[nodiscard]] virtual const Ptrs<Device>&         GetGpuDevices() const noexcept = 0;
    [[nodiscard]] virtual Ptr<Device>                 GetNextGpuDevice(const Device& device) const noexcept = 0;
    [[nodiscard]] virtual Ptr<Device>                 GetSoftwareGpuDevice() const noexcept = 0;
    [[nodiscard]] virtual const Device::Capabilities& GetDeviceCapabilities() const noexcept = 0;
    [[nodiscard]] virtual std::string                 ToString() const = 0;
    
    virtual ~System() = default;
};

} // namespace Methane::Graphics
