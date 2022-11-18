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

FILE: Methane/Graphics/RHI/IDevice.h
Methane system and device interfaces: describes system graphics devices,
used to create graphics context for rendering.

******************************************************************************/

#pragma once

#include "IObject.h"

#include <Methane/Data/IEmitter.h>
#include <Methane/Memory.hpp>

#include <array>
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

/*
enum class DeviceFeatures : uint32_t
{
    Unknown              = 0U,
    BasicRendering       = 1U << 0U,
    AnisotropicFiltering = 1U << 2U,
    ImageCubeArray       = 1U << 3U,
    All                  = ~0U,
};
*/

union DeviceFeatures
{
    struct
    {
        bool present_to_window;
        bool anisotropic_filtering;
        bool image_cube_array;
    };

    uint32_t mask { 0U };

    DeviceFeatures() = default;
    DeviceFeatures(uint32_t mask) : mask(mask) { }
};

struct DeviceCaps
{
    DeviceFeatures features              { ~0U };
    uint32_t       render_queues_count   { 1U };
    uint32_t       transfer_queues_count { 1U };

    DeviceCaps& SetFeatures(DeviceFeatures new_features) noexcept;
    DeviceCaps& SetRenderQueuesCount(uint32_t new_render_queues_count) noexcept;
    DeviceCaps& SetTransferQueuesCount(uint32_t new_transfer_queues_count) noexcept;
};

struct IDevice;

struct IDeviceCallback
{
    virtual void OnDeviceRemovalRequested(IDevice& device) = 0;
    virtual void OnDeviceRemoved(IDevice& device) = 0;

    virtual ~IDeviceCallback() = default;
};

struct IDevice
    : virtual IObject // NOSONAR
    , virtual Data::IEmitter<IDeviceCallback> // NOSONAR
{
    using Features = DeviceFeatures;
    using Capabilities = DeviceCaps;

    [[nodiscard]] virtual const std::string&  GetAdapterName() const noexcept = 0;
    [[nodiscard]] virtual bool                IsSoftwareAdapter() const noexcept = 0;
    [[nodiscard]] virtual const Capabilities& GetCapabilities() const noexcept = 0;
    [[nodiscard]] virtual std::string         ToString() const = 0;
};

struct ISystem
{
    static NativeApi GetNativeApi() noexcept;

    [[nodiscard]] static ISystem& Get();

    virtual void CheckForChanges() = 0;
    [[nodiscard]] virtual const Ptrs<IDevice>& UpdateGpuDevices(const DeviceCaps& required_device_caps = {}) = 0;
    [[nodiscard]] virtual const Ptrs<IDevice>& UpdateGpuDevices(const Platform::AppEnvironment& app_env,
                                                                     const DeviceCaps& required_device_caps = {}) = 0;
    [[nodiscard]] virtual const Ptrs<IDevice>& GetGpuDevices() const noexcept = 0;
    [[nodiscard]] virtual Ptr<IDevice>         GetNextGpuDevice(const IDevice& device) const noexcept = 0;
    [[nodiscard]] virtual Ptr<IDevice>         GetSoftwareGpuDevice() const noexcept = 0;
    [[nodiscard]] virtual const DeviceCaps&    GetDeviceCapabilities() const noexcept = 0;
    [[nodiscard]] virtual std::string          ToString() const = 0;
    
    virtual ~ISystem() = default;
};

} // namespace Methane::Graphics::Rhi
