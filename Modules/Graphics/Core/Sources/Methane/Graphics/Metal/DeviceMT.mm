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

FILE: Methane/Graphics/Metal/DeviceMT.mm
Metal implementation of the device interface.

******************************************************************************/

#include "DeviceMT.hh"

#include <Methane/Platform/MacOS/Types.hh>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <algorithm>
#include <magic_enum.hpp>

namespace Methane::Graphics
{

Device::Features DeviceMT::GetSupportedFeatures(const id<MTLDevice>&)
{
    META_FUNCTION_TASK();
    Device::Features supported_features = Device::Features::BasicRendering;
    return supported_features;
}

DeviceMT::DeviceMT(const id<MTLDevice>& mtl_device, const Capabilities& capabilities)
    : DeviceBase(MacOS::ConvertFromNsType<NSString, std::string>(mtl_device.name), false, capabilities)
    , m_mtl_device(mtl_device)
{
    META_FUNCTION_TASK();
}

DeviceMT::~DeviceMT()
{
    META_FUNCTION_TASK();

    [m_mtl_device release];
}

System& System::Get()
{
    META_FUNCTION_TASK();
    static SystemMT s_system;
    return s_system;
}

SystemMT::~SystemMT()
{
    META_FUNCTION_TASK();
    if (m_device_observer != nil)
    {
        MTLRemoveDeviceObserver(m_device_observer);
    }
}

const Ptrs<Device>& SystemMT::UpdateGpuDevices(const Platform::AppEnvironment&, const Device::Capabilities& required_device_caps)
{
    META_FUNCTION_TASK();
    return UpdateGpuDevices(required_device_caps);
}

const Ptrs<Device>& SystemMT::UpdateGpuDevices(const Device::Capabilities& required_device_caps)
{
    META_FUNCTION_TASK();
    if (m_device_observer != nil)
    {
        MTLRemoveDeviceObserver(m_device_observer);
    }

    SetDeviceCapabilities(required_device_caps);
    ClearDevices();
    
    NSArray<id<MTLDevice>>* mtl_devices = MTLCopyAllDevicesWithObserver(&m_device_observer,
                                                                        ^(id<MTLDevice> device, MTLDeviceNotificationName device_notification)
                                                                        {
                                                                            OnDeviceNotification(device, device_notification);
                                                                        });
    
    for(id<MTLDevice> mtl_device in mtl_devices)
    {
        AddDevice(mtl_device, required_device_caps);
    }
    
    return GetGpuDevices();
}

void SystemMT::OnDeviceNotification(id<MTLDevice> mtl_device, MTLDeviceNotificationName device_notification)
{
    META_FUNCTION_TASK();
    if (device_notification == MTLDeviceWasAddedNotification)
    {
        AddDevice(mtl_device);
    }
    else
    {
        const Ptr<Device>& device_ptr = FindMetalDevice(mtl_device);
        META_CHECK_ARG_NOT_NULL_DESCR(device_ptr, "no device object found");

        if (device_notification == MTLDeviceRemovalRequestedNotification)
            RequestRemoveDevice(*device_ptr);
        else if (device_notification == MTLDeviceWasRemovedNotification)
            RemoveDevice(*device_ptr);
    }
}

void SystemMT::AddDevice(const id<MTLDevice>& mtl_device)
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;

    Device::Features device_supported_features = DeviceMT::GetSupportedFeatures(mtl_device);
    if (!magic_enum::flags::enum_contains(device_supported_features& GetDeviceCapabilities().features))
        return;

    SystemBase::AddDevice(std::make_shared<DeviceMT>(mtl_device, GetDeviceCapabilities()));
}

const Ptr<Device>& SystemMT::FindMetalDevice(const id<MTLDevice>& mtl_device) const
{
    META_FUNCTION_TASK();
    const Ptrs<Device>& devices = GetGpuDevices();
    const auto device_it = std::find_if(devices.begin(), devices.end(),
                                        [mtl_device](const Ptr<Device>& device_ptr)
                                        {
                                            META_CHECK_ARG_NOT_NULL(device_ptr);
                                            DeviceMT& metal_device = static_cast<DeviceMT&>(*device_ptr);
                                            return metal_device.GetNativeDevice() == mtl_device;
                                        });
    
    static const Ptr<Device> s_empty_device_ptr;
    return device_it != devices.end() ? *device_it : s_empty_device_ptr;
}

} // namespace Methane::Graphics
