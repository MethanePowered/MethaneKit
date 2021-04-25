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
        Unknown                 = 0U,
        BasicRendering          = 1U << 0U,
        TextureAndSamplerArrays = 1U << 1U,
        All                     = ~0U,
    };

    [[nodiscard]] virtual const std::string& GetAdapterName() const noexcept = 0;
    [[nodiscard]] virtual bool               IsSoftwareAdapter() const noexcept = 0;
    [[nodiscard]] virtual Features           GetSupportedFeatures() const noexcept = 0;
    [[nodiscard]] virtual std::string        ToString() const = 0;
};

struct System
{
    [[nodiscard]] static System& Get();

    virtual void CheckForChanges() = 0;
    [[nodiscard]] virtual const Ptrs<Device>& UpdateGpuDevices(Device::Features supported_features = Device::Features::All) = 0;
    [[nodiscard]] virtual const Ptrs<Device>& GetGpuDevices() const = 0;
    [[nodiscard]] virtual Ptr<Device>         GetNextGpuDevice(const Device& device) const = 0;
    [[nodiscard]] virtual Ptr<Device>         GetSoftwareGpuDevice() const = 0;
    [[nodiscard]] virtual Device::Features    GetGpuSupportedFeatures() const = 0;
    [[nodiscard]] virtual std::string         ToString() const = 0;
    
    virtual ~System() = default;
};

} // namespace Methane::Graphics
