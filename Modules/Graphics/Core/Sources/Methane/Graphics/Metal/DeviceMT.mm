/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Metal/DeviceMT.mm
Metal implementation of the device interface.

******************************************************************************/

#include "DeviceMT.hh"

#include <Methane/Graphics/Instrumentation.h>
#include <Methane/Platform/MacOS/Types.hh>

#include <algorithm>

namespace Methane
{
namespace Graphics
{

Device::Feature::Mask DeviceMT::GetSupportedFeatures(const id<MTLDevice>& mtl_device)
{
    ITT_FUNCTION_TASK();
    Device::Feature::Mask supported_featues = Device::Feature::Value::BasicRendering;
    return supported_featues;
}

DeviceMT::DeviceMT(const id<MTLDevice>& mtl_device)
    : DeviceBase(MacOS::ConvertFromNSType<NSString, std::string>(mtl_device.name), false,
                 GetSupportedFeatures(mtl_device))
    , m_mtl_device(mtl_device)
{
    ITT_FUNCTION_TASK();
}

DeviceMT::~DeviceMT()
{
    ITT_FUNCTION_TASK();

    [m_mtl_device release];
}

System& System::Get()
{
    ITT_FUNCTION_TASK();
    static SystemMT s_system;
    return s_system;
}

SystemMT::~SystemMT()
{
    ITT_FUNCTION_TASK();
    if (m_device_observer != nil)
    {
        MTLRemoveDeviceObserver(m_device_observer);
    }
}

const Devices& SystemMT::UpdateGpuDevices(Device::Feature::Mask supported_features)
{
    ITT_FUNCTION_TASK();
    if (m_device_observer != nil)
    {
        MTLRemoveDeviceObserver(m_device_observer);
    }
    
    m_supported_features = supported_features;
    m_devices.clear();
    
    NSArray<id<MTLDevice>>* mtl_devices = MTLCopyAllDevicesWithObserver(&m_device_observer,
                                                                        ^(id<MTLDevice> device, MTLDeviceNotificationName device_notification)
                                                                        {
                                                                            OnDeviceNotification(device, device_notification);
                                                                        });
    
    for(id<MTLDevice> mtl_device in mtl_devices)
    {
        AddDevice(mtl_device);
    }
    
    return m_devices;
}

void SystemMT::OnDeviceNotification(id<MTLDevice> mtl_device, MTLDeviceNotificationName device_notification)
{
    ITT_FUNCTION_TASK();
    if (device_notification == MTLDeviceWasAddedNotification)
    {
        AddDevice(mtl_device);
    }
    else if (device_notification == MTLDeviceRemovalRequestedNotification)
    {
        NotifyDevice(mtl_device, Device::Notification::RemoveRequested);
    }
    else if (device_notification == MTLDeviceWasRemovedNotification)
    {
        NotifyDevice(mtl_device, Device::Notification::Removed);
    }
}

void SystemMT::NotifyDevice(const id<MTLDevice>& mtl_device, Device::Notification device_notification)
{
    ITT_FUNCTION_TASK();
    const Device::Ptr& sp_device = FindMetalDevice(mtl_device);
    if (!sp_device)
    {
        assert(0);
        return;
    }
    sp_device->Notify(device_notification);
}

void SystemMT::AddDevice(const id<MTLDevice>& mtl_device)
{
    ITT_FUNCTION_TASK();
    Device::Feature::Mask device_supported_features = DeviceMT::GetSupportedFeatures(mtl_device);
    if (!(device_supported_features & m_supported_features))
        return;
    
    Device::Ptr sp_device = std::make_shared<DeviceMT>(mtl_device);
    m_devices.push_back(sp_device);
}

const Device::Ptr& SystemMT::FindMetalDevice(const id<MTLDevice>& mtl_device) const
{
    ITT_FUNCTION_TASK();
    const auto device_it = std::find_if(m_devices.begin(), m_devices.end(),
                                        [mtl_device](const Device::Ptr& sp_device)
                                        {
                                            assert(!!sp_device);
                                            if (!sp_device) return false;
                                            DeviceMT& metal_device = static_cast<DeviceMT&>(*sp_device);
                                            return metal_device.GetNativeDevice() == mtl_device;
                                        });
    
    static const Device::Ptr sp_empty_device;
    return device_it != m_devices.end() ? *device_it : sp_empty_device;
}

} // namespace Graphics
} // namespace Methane
