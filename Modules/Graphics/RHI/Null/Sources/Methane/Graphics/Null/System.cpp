/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Null/System.cpp
Null implementation of the system interface.

******************************************************************************/

#include <Methane/Graphics/Null/System.h>
#include <Methane/Graphics/Null/Device.h>

namespace Methane::Graphics::Rhi
{

ISystem& ISystem::Get()
{
    static const auto s_system_ptr = std::make_shared<Null::System>();
    return *s_system_ptr;
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::Null
{

void System::CheckForChanges()
{
    /* Intentionally unimplemented */
}

const Ptrs<Rhi::IDevice>& System::UpdateGpuDevices(const Methane::Platform::AppEnvironment&, const Rhi::DeviceCaps& required_device_caps)
{
    return UpdateGpuDevices(required_device_caps);
}

const Ptrs<Rhi::IDevice>& System::UpdateGpuDevices(const Rhi::DeviceCaps& required_device_caps)
{
    META_FUNCTION_TASK();
    SetDeviceCapabilities(required_device_caps);
    ClearDevices();

    AddDevice(std::make_shared<Device>("Test GPU 1", false, required_device_caps));
    AddDevice(std::make_shared<Device>("Test GPU 1", false, Rhi::DeviceCaps()));
    AddDevice(std::make_shared<Device>("Test WARP",  true,  required_device_caps));

    return GetGpuDevices();
}

} // namespace Methane::Graphics::Null
