/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/DeviceVK.mm
Vulkan implementation of the device interface.

******************************************************************************/

#include "DeviceVK.h"

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace Methane::Graphics
{

static const char* g_vk_app_name = "Methane Application";
static const char* g_vk_engine_name = "Methane Kit";

static vk::Instance CreateVulkanInstance(vk::DynamicLoader& vk_loader)
{
    META_FUNCTION_TASK();

    VULKAN_HPP_DEFAULT_DISPATCHER.init(vk_loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));

    vk::ApplicationInfo vk_app_info(g_vk_app_name, 1, g_vk_engine_name, 1, VK_API_VERSION_1_1);

    // TODO: set Vulkan layers
    vk::InstanceCreateInfo vk_instance_create_info({}, &vk_app_info);

    vk::Instance vk_instance = vk::createInstance(vk_instance_create_info);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vk_instance);

    return vk_instance;
}

DeviceVK::DeviceVK()
    : DeviceBase("", false, Device::Features::BasicRendering)
{
    META_FUNCTION_TASK();
}

System& System::Get()
{
    META_FUNCTION_TASK();
    static SystemVK s_system;
    return s_system;
}

SystemVK::SystemVK()
    : m_vk_instance(CreateVulkanInstance(m_vk_loader))
{
    META_FUNCTION_TASK();
}

SystemVK::~SystemVK()
{
    META_FUNCTION_TASK();
    m_vk_instance.destroy();
}

void SystemVK::CheckForChanges()
{
    META_FUNCTION_TASK();
    META_FUNCTION_NOT_IMPLEMENTED();
}

const Ptrs<Device>& SystemVK::UpdateGpuDevices(Device::Features supported_features)
{
    META_FUNCTION_TASK();
    SetGpuSupportedFeatures(supported_features);
    ClearDevices();
    return GetGpuDevices();
}

} // namespace Methane::Graphics
