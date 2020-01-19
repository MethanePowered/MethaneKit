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

FILE: Methane/Graphics/Vulkan/DeviceVK.h
Vulkan implementation of the device interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/DeviceBase.h>

namespace Methane::Graphics
{

class DeviceVK final : public DeviceBase
{
public:
    DeviceVK();
    ~DeviceVK() override;
};

class SystemVK final : public SystemBase
{
public:
    ~SystemVK() override;
    
    void           CheckForChanges() override {}
    const Ptrs<Device>& UpdateGpuDevices(Device::Feature::Mask supported_features) override;
};

} // namespace Methane::Graphics
