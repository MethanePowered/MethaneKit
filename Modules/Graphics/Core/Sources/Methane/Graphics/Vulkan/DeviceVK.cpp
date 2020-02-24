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

FILE: Methane/Graphics/Vulkan/DeviceVK.mm
Vulkan implementation of the device interface.

******************************************************************************/

#include "DeviceVK.h"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

DeviceVK::DeviceVK()
    : DeviceBase("", false, Device::Feature::Value::BasicRendering)
{
    ITT_FUNCTION_TASK();
}

DeviceVK::~DeviceVK()
{
    ITT_FUNCTION_TASK();
}

System& System::Get()
{
    ITT_FUNCTION_TASK();
    static SystemVK s_system;
    return s_system;
}

SystemVK::~SystemVK()
{
    ITT_FUNCTION_TASK();
}

const Ptrs<Device>& SystemVK::UpdateGpuDevices(Device::Feature::Mask supported_features)
{
    ITT_FUNCTION_TASK();
    SetGpuSupportedFeatures(supported_features);
    ClearDevices();
    return GetGpuDevices();
}

} // namespace Methane::Graphics
