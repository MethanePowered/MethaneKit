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

FILE: Methane/Graphics/Base/Device.cpp
Base implementation of the device interface.

******************************************************************************/

#include <Methane/Graphics/Base/Device.h>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <sstream>

namespace Methane::Graphics
{

Rhi::NativeApi Rhi::ISystem::GetNativeApi() noexcept
{
#if defined METHANE_GFX_METAL
    return NativeApi::Metal;
#elif defined METHANE_GFX_DIRECTX
    return NativeApi::DirectX;
#elif defined METHANE_GFX_VULKAN
    return NativeApi::Vulkan;
#else
    return NativeApi::Undefined;
#endif
}

} // namespace Methane::Graphics

namespace Methane::Graphics::Base
{

Device::Device(const std::string& adapter_name, bool is_software_adapter, const Capabilities& capabilities)
    : m_system_ptr(static_cast<System&>(Rhi::ISystem::Get()).GetPtr()) // NOSONAR
    , m_adapter_name(adapter_name)
    , m_is_software_adapter(is_software_adapter)
    , m_capabilities(capabilities)
{
    META_FUNCTION_TASK();
}

std::string Device::ToString() const
{
    META_FUNCTION_TASK();
    return fmt::format("GPU \"{}\"", GetAdapterName());
}

void Device::OnRemovalRequested()
{
    META_FUNCTION_TASK();
    Data::Emitter<Rhi::IDeviceCallback>::Emit(&Rhi::IDeviceCallback::OnDeviceRemovalRequested, std::ref(*this));
}

void Device::OnRemoved()
{
    META_FUNCTION_TASK();
    Data::Emitter<Rhi::IDeviceCallback>::Emit(&Rhi::IDeviceCallback::OnDeviceRemoved, std::ref(*this));
}

void System::RequestRemoveDevice(Rhi::IDevice& device) const
{
    META_FUNCTION_TASK();
    static_cast<Device&>(device).OnRemovalRequested();
}

void System::RemoveDevice(Rhi::IDevice& device)
{
    META_FUNCTION_TASK();
    const auto device_it = std::find_if(m_devices.begin(), m_devices.end(),
        [&device](const Ptr<Rhi::IDevice>& device_ptr)
        { return std::addressof(device) == std::addressof(*device_ptr); }
    );
    if (device_it == m_devices.end())
        return;

    Ptr<Rhi::IDevice> device_ptr = *device_it;
    m_devices.erase(device_it);
    static_cast<Device&>(device).OnRemoved();
}

Ptr<Rhi::IDevice> System::GetNextGpuDevice(const Rhi::IDevice& device) const noexcept
{
    META_FUNCTION_TASK();
    Ptr<Rhi::IDevice> next_device_ptr;
    
    if (m_devices.empty())
        return next_device_ptr;
    
    auto device_it = std::find_if(m_devices.begin(), m_devices.end(),
                                  [&device](const Ptr<Rhi::IDevice>& system_device_ptr)
                                  { return std::addressof(device) == system_device_ptr.get(); });
    if (device_it == m_devices.end())
        return next_device_ptr;
    
    return device_it == m_devices.end() - 1 ? m_devices.front() : *(device_it + 1);
}

Ptr<Rhi::IDevice> System::GetSoftwareGpuDevice() const noexcept
{
    META_FUNCTION_TASK();
    auto sw_device_it = std::find_if(m_devices.begin(), m_devices.end(),
        [](const Ptr<Rhi::IDevice>& system_device_ptr)
        { return system_device_ptr && system_device_ptr->IsSoftwareAdapter(); });

    return sw_device_it != m_devices.end() ? *sw_device_it : Ptr<Rhi::IDevice>();
}

std::string System::ToString() const
{
    META_FUNCTION_TASK();
    std::stringstream ss;
    ss << "Available graphics devices:" << std::endl;
    for(const Ptr<Rhi::IDevice>& device_ptr : m_devices)
    {
        META_CHECK_ARG_NOT_NULL(device_ptr);
        ss << "  - " << device_ptr->ToString() << ";" << std::endl;
    }
    return ss.str();
}

} // namespace Methane::Graphics::Base