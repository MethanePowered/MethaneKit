/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
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

#include <Methane/Memory.hpp>

#include <array>
#include <functional>

namespace Methane::Graphics
{

struct Device : virtual Object
{
    enum class Notification : uint32_t
    {
        RemoveRequested = 0,
        Removed,
    };
    
    using NotificationCallback = std::function<void(Device&, Notification)>;

    struct Feature
    {
        using Mask = uint32_t;
        enum Value : Mask
        {
            Unknown                 = 0u,
            BasicRendering          = 1u << 0u,
            TextureAndSamplerArrays = 1u << 1u,
            All                     = ~0u,
        };

        using Values = std::array<Value, 2>;
        static constexpr const Values values = { BasicRendering, TextureAndSamplerArrays };
        
        static std::string ToString(Value feature) noexcept;
        static std::string ToString(Mask features) noexcept;

        Feature()  = delete;
        ~Feature() = delete;
    };

    virtual const std::string& GetAdapterName() const noexcept = 0;
    virtual bool               IsSoftwareAdapter() const noexcept = 0;
    virtual Feature::Mask      GetSupportedFeatures() const noexcept = 0;
    virtual void               SetNotificationCallback(const NotificationCallback& callback) = 0;
    virtual void               Notify(Notification notification) = 0;
    virtual std::string        ToString() const noexcept = 0;
};

struct System
{
    static System& Get();

    virtual void                  CheckForChanges() = 0;
    virtual const Ptrs<Device>&   UpdateGpuDevices(Device::Feature::Mask supported_features = Device::Feature::Value::All) = 0;
    virtual const Ptrs<Device>&   GetGpuDevices() const = 0;
    virtual Ptr<Device>           GetNextGpuDevice(const Device& device) const = 0;
    virtual Ptr<Device>           GetSoftwareGpuDevice() const = 0;
    virtual Device::Feature::Mask GetGpuSupportedFeatures() const = 0;
    virtual std::string           ToString() const noexcept = 0;
    
    virtual ~System() = default;
};

} // namespace Methane::Graphics
