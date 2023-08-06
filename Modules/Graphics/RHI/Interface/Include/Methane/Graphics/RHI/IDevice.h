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
Methane device interface, used to create graphics context for rendering.

******************************************************************************/

#pragma once

#include "IObject.h"

#include <Methane/Data/IEmitter.h>
#include <Methane/Data/EnumMask.hpp>
#include <Methane/Memory.hpp>

#include <functional>

namespace tf
{
// TaskFlow Executor class forward declaration from <taskflow/core/executor.hpp>
class Executor;
}

namespace Methane::Platform
{
struct AppEnvironment;
}

namespace Methane::Graphics::Rhi
{

enum class DeviceFeature : uint32_t
{
    PresentToWindow,
    AnisotropicFiltering,
    ImageCubeArray
};

using DeviceFeatureMask = Data::EnumMask<DeviceFeature>;

struct DeviceCaps
{
    DeviceFeatureMask features{
        DeviceFeature::PresentToWindow,
        DeviceFeature::AnisotropicFiltering,
        DeviceFeature::ImageCubeArray,
    };
    uint32_t render_queues_count   { 1U };
    uint32_t transfer_queues_count { 1U };
    uint32_t compute_queues_count  { 1U };

    DeviceCaps& SetFeatures(DeviceFeatureMask new_features) noexcept;
    DeviceCaps& SetRenderQueuesCount(uint32_t new_render_queues_count) noexcept;
    DeviceCaps& SetTransferQueuesCount(uint32_t new_transfer_queues_count) noexcept;
    DeviceCaps& SetComputeQueuesCount(uint32_t new_compute_queues_count) noexcept;
};

struct IDevice;

struct IDeviceCallback
{
    virtual void OnDeviceRemovalRequested(IDevice& device) = 0;
    virtual void OnDeviceRemoved(IDevice& device) = 0;

    virtual ~IDeviceCallback() = default;
};

struct IRenderContext;
struct IComputeContext;
struct RenderContextSettings;
struct ComputeContextSettings;

struct IDevice
    : virtual IObject // NOSONAR
    , virtual Data::IEmitter<IDeviceCallback> // NOSONAR
{
    using FeatureMask  = DeviceFeatureMask;
    using Feature      = DeviceFeature;
    using Capabilities = DeviceCaps;

    [[nodiscard]] virtual Ptr<IRenderContext>  CreateRenderContext(const Platform::AppEnvironment& env, tf::Executor& parallel_executor, const RenderContextSettings& settings) = 0;
    [[nodiscard]] virtual Ptr<IComputeContext> CreateComputeContext(tf::Executor& parallel_executor, const ComputeContextSettings& settings) = 0;
    [[nodiscard]] virtual const std::string&   GetAdapterName() const noexcept = 0;
    [[nodiscard]] virtual bool                 IsSoftwareAdapter() const noexcept = 0;
    [[nodiscard]] virtual const Capabilities&  GetCapabilities() const noexcept = 0;
    [[nodiscard]] virtual std::string          ToString() const = 0;
};

} // namespace Methane::Graphics::Rhi
