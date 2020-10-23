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

#include <Methane/Instrumentation.h>
#include <Methane/Platform/MacOS/Types.hh>

#include <algorithm>

namespace Methane::Graphics
{

Device::Feature::Mask DeviceMT::GetSupportedFeatures(const id<MTLDevice>&)
{
    META_FUNCTION_TASK();
    Device::Feature::Mask supported_features = Device::Feature::Value::BasicRendering;
    return supported_features;
}

DeviceMT::DeviceMT(const id<MTLDevice>& mtl_device)
    : DeviceBase(MacOS::ConvertFromNsType<NSString, std::string>(mtl_device.name), false,
                 GetSupportedFeatures(mtl_device))
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

const Ptrs<Device>& SystemMT::UpdateGpuDevices(Device::Feature::Mask supported_features)
{
    META_FUNCTION_TASK();
    if (m_device_observer != nil)
    {
        MTLRemoveDeviceObserver(m_device_observer);
    }

    SetGpuSupportedFeatures(supported_features);
    ClearDevices();
    
    NSArray<id<MTLDevice>>* mtl_devices = MTLCopyAllDevicesWithObserver(&m_device_observer,
                                                                        ^(id<MTLDevice> device, MTLDeviceNotificationName device_notification)
                                                                        {
                                                                            OnDeviceNotification(device, device_notification);
                                                                        });
    
    for(id<MTLDevice> mtl_device in mtl_devices)
    {
        AddDevice(mtl_device);
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
        if (!device_ptr)
            throw std::logic_error("No device object found");

        if (device_notification == MTLDeviceRemovalRequestedNotification)
            RequestRemoveDevice(*device_ptr);
        else if (device_notification == MTLDeviceWasRemovedNotification)
            RemoveDevice(*device_ptr);
    }
}

void SystemMT::AddDevice(const id<MTLDevice>& mtl_device)
{
    META_FUNCTION_TASK();
    Device::Feature::Mask device_supported_features = DeviceMT::GetSupportedFeatures(mtl_device);
    if (!(device_supported_features & GetGpuSupportedFeatures()))
        return;

    SystemBase::AddDevice(std::make_shared<DeviceMT>(mtl_device));
}

const Ptr<Device>& SystemMT::FindMetalDevice(const id<MTLDevice>& mtl_device) const
{
    META_FUNCTION_TASK();
    const Ptrs<Device>& devices = GetGpuDevices();
    const auto device_it = std::find_if(devices.begin(), devices.end(),
                                        [mtl_device](const Ptr<Device>& device_ptr)
                                        {
                                            assert(!!device_ptr);
                                            if (!device_ptr) return false;
                                            DeviceMT& metal_device = static_cast<DeviceMT&>(*device_ptr);
                                            return metal_device.GetNativeDevice() == mtl_device;
                                        });
    
    static const Ptr<Device> s_empty_device_ptr;
    return device_it != devices.end() ? *device_it : s_empty_device_ptr;
}

} // namespace Methane::Graphics
