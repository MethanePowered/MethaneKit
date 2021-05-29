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
#include "PlatformVK.h"

#include <Methane/Platform/Utils.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <vector>
#include <sstream>

//#define VULKAN_VALIDATION_BEST_PRACTICES_ENABLED

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace Methane::Graphics
{

static const std::string g_vk_app_name    = "Methane Application";
static const std::string g_vk_engine_name = "Methane Kit";

static const std::string g_vk_validation_layer        = "VK_LAYER_KHRONOS_validation";
static const std::string g_vk_debug_utils_extension   = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
static const std::string g_vk_validation_extension    = "VK_EXT_validation_features";

static std::vector<const char*> GetEnabledLayers(const std::vector<std::string>& layers)
{
    META_FUNCTION_TASK();

#ifndef NDEBUG
    const std::vector<vk::LayerProperties> layer_properties = vk::enumerateInstanceLayerProperties();
#endif

    std::vector<char const *> enabled_layers;
    enabled_layers.reserve(layers.size() );
    for (const std::string& layer : layers)
    {
        assert(std::find_if(layer_properties.begin(), layer_properties.end(),
                            [layer](const vk::LayerProperties& lp) { return layer == lp.layerName; }
                            ) != layer_properties.end() );
        enabled_layers.push_back(layer.data() );
    }

#ifndef NDEBUG
    if (std::find(layers.begin(), layers.end(), g_vk_validation_layer) == layers.end() &&
        std::find_if(layer_properties.begin(), layer_properties.end(),
                     [](const vk::LayerProperties& lp) { return g_vk_validation_layer == lp.layerName; }
                     ) != layer_properties.end())
    {
        enabled_layers.push_back(g_vk_validation_layer.c_str());
    }
#endif

    return enabled_layers;
}

static std::vector<char const *> GetEnabledExtensions(const std::vector<std::string>& extensions)
{
    META_FUNCTION_TASK();

#ifndef NDEBUG
    const std::vector<vk::ExtensionProperties>& extension_properties = vk::enumerateInstanceExtensionProperties();
#endif

    std::vector<char const *> enabled_extensions;
    enabled_extensions.reserve(extensions.size());

    for (const std::string& ext : extensions)
    {
        assert(std::find_if(extension_properties.begin(), extension_properties.end(),
                            [ext](const vk::ExtensionProperties& ep) { return ext == ep.extensionName; }
                            ) != extension_properties.end());
        enabled_extensions.push_back(ext.data() );
    }

#ifndef NDEBUG
    const auto add_enabled_extension = [&extensions, &enabled_extensions, &extension_properties](const std::string& extension)
    {
        if (std::find(extensions.begin(), extensions.end(), extension) == extensions.end() &&
            std::find_if(extension_properties.begin(), extension_properties.end(),
                         [&extension](const vk::ExtensionProperties& ep) { return extension == ep.extensionName; }
            ) != extension_properties.end())
        {
            enabled_extensions.push_back(extension.c_str());
        }
    };

    add_enabled_extension(g_vk_debug_utils_extension);

#ifdef VULKAN_VALIDATION_BEST_PRACTICES_ENABLED
    add_enabled_extension(g_vk_validation_extension);
#endif

#endif

    return enabled_extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
                                                           VkDebugUtilsMessageTypeFlagsEXT             message_types,
                                                           const VkDebugUtilsMessengerCallbackDataEXT* callback_data_ptr,
                                                           void* /*user_data_ptr*/) // NOSONAR
{
    META_FUNCTION_TASK();

#ifndef NDEBUG

    if (callback_data_ptr->messageIdNumber == 648835635) // UNASSIGNED-khronos-Validation-debug-build-warning-message
        return VK_FALSE;

    if (callback_data_ptr->messageIdNumber == 767975156) // UNASSIGNED-BestPractices-vkCreateInstance-specialise-extension
        return VK_FALSE;

#endif

    std::stringstream ss;
    ss << vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(message_severity)) << " "
       << vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(message_types)) << ":" << std::endl;
    ss << "\t- messageIDName:   " << callback_data_ptr->pMessageIdName << std::endl;
    ss << "\t- messageIdNumber: "  << callback_data_ptr->messageIdNumber << std::endl;
    ss << "\t- message:         " << callback_data_ptr->pMessage << std::endl;
    if (callback_data_ptr->queueLabelCount > 0)
    {
        ss << "\t- Queue Labels:" << std::endl;
        for (uint32_t i = 0; i < callback_data_ptr->queueLabelCount; i++)
        {
            ss << "\t\t- " << callback_data_ptr->pQueueLabels[i].pLabelName << std::endl;
        }
    }
    if (callback_data_ptr->cmdBufLabelCount > 0)
    {
        ss << "\t- CommandBuffer Labels:" << std::endl;
        for (uint32_t i = 0; i < callback_data_ptr->cmdBufLabelCount; i++)
        {
            ss << "\t\t- " << callback_data_ptr->pCmdBufLabels[i].pLabelName << std::endl;
        }
    }
    if (callback_data_ptr->objectCount > 0)
    {
        ss << "\t- Objects:" << std::endl;
        for (uint32_t i = 0; i < callback_data_ptr->objectCount; i++)
        {
            ss << "\t\t- Object " << i << ":" << std::endl;
            ss << "\t\t\t- objectType:   " << vk::to_string( static_cast<vk::ObjectType>( callback_data_ptr->pObjects[i].objectType ) ) << std::endl;
            ss << "\t\t\t- objectHandle: " << callback_data_ptr->pObjects[i].objectHandle << std::endl;
            if (callback_data_ptr->pObjects[i].pObjectName)
                ss << "\t\t\t- objectName:   " << callback_data_ptr->pObjects[i].pObjectName << std::endl;
        }
    }

    Platform::PrintToDebugOutput(ss.str());
    return VK_TRUE;
}

#ifdef NDEBUG
using InstanceCreateInfoChain = vk::StructureChain<vk::InstanceCreateInfo>;
#elif defined(VULKAN_VALIDATION_BEST_PRACTICES_ENABLED)
using InstanceCreateInfoChain = vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT, vk::ValidationFeaturesEXT>;
#else
using InstanceCreateInfoChain = vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT>;
#endif

InstanceCreateInfoChain MakeInstanceCreateInfoChain(const vk::ApplicationInfo& vk_app_info,
                                                    const std::vector<char const *>& layers,
                                                    const std::vector<char const *>& extensions)
{
    META_FUNCTION_TASK();

#if defined( NDEBUG )
    return InstanceCreateInfoChain({ {}, &vk_app_info, layers, extensions });
#else
    vk::DebugUtilsMessageSeverityFlagsEXT severity_flags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
    );
    vk::DebugUtilsMessageTypeFlagsEXT message_type_flags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
    );
#ifdef VULKAN_VALIDATION_BEST_PRACTICES_ENABLED
    const vk::ValidationFeatureEnableEXT validation_feature_enable = vk::ValidationFeatureEnableEXT::eBestPractices;
#endif
    return InstanceCreateInfoChain(
        { {}, &vk_app_info, layers, extensions },
        { {}, severity_flags, message_type_flags, &DebugUtilsMessengerCallback }
#ifdef VULKAN_VALIDATION_BEST_PRACTICES_ENABLED
        , { validation_feature_enable }
#endif
    );
#endif
}

static vk::Instance CreateVulkanInstance(const vk::DynamicLoader& vk_loader,
                                         const std::vector<std::string>& layers = {},
                                         const std::vector<std::string>& extensions = {},
                                         uint32_t vk_api_version = VK_API_VERSION_1_1)
{
    META_FUNCTION_TASK();

    VULKAN_HPP_DEFAULT_DISPATCHER.init(vk_loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));

    constexpr uint32_t engine_version = METHANE_VERSION_MAJOR * 10 + METHANE_VERSION_MINOR;
    const std::vector<char const *> enabled_layers     = GetEnabledLayers(layers);
    const std::vector<char const *> enabled_extensions = GetEnabledExtensions(extensions);
    const vk::ApplicationInfo vk_app_info(g_vk_app_name.c_str(), 1, g_vk_engine_name.c_str(), engine_version, vk_api_version);
    const vk::InstanceCreateInfo vk_instance_create_info = MakeInstanceCreateInfoChain(vk_app_info, enabled_layers, enabled_extensions).get<vk::InstanceCreateInfo>();

    vk::Instance vk_instance = vk::createInstance(vk_instance_create_info);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vk_instance);
    return vk_instance;
}

template<bool exact_flags_matching>
std::optional<uint32_t> FindQueueFamily(const std::vector<vk::QueueFamilyProperties>& vk_queue_family_properties,
                                        vk::QueueFlagBits queue_flags, uint32_t queues_count,
                                        const std::vector<uint32_t>& reserved_queues_count_per_family,
                                        const vk::PhysicalDevice& vk_physical_device = {}, const vk::SurfaceKHR& vk_present_surface = {})
{
    META_FUNCTION_TASK();
    for(size_t family_index = 0; family_index < vk_queue_family_properties.size(); ++family_index)
    {
        const vk::QueueFamilyProperties& vk_family_props = vk_queue_family_properties[family_index];
        if constexpr (exact_flags_matching)
        {
            if (vk_family_props.queueFlags != queue_flags)
                continue;
        }
        else
        {
            if (!(vk_family_props.queueFlags & queue_flags))
                continue;
        }

        if (vk_family_props.queueCount < queues_count + reserved_queues_count_per_family.at(family_index))
            continue;

        if (queue_flags == vk::QueueFlagBits::eGraphics && vk_physical_device && vk_present_surface &&
            !vk_physical_device.getSurfaceSupportKHR(static_cast<uint32_t>(family_index), vk_present_surface))
            continue;

        return static_cast<uint32_t>(family_index);
    }

    return {};
}

static std::optional<uint32_t> FindQueueFamily(const std::vector<vk::QueueFamilyProperties>& vk_queue_family_properties,
                                               vk::QueueFlagBits queue_flags, uint32_t queues_count,
                                               const std::vector<uint32_t>& reserved_queues_count_per_family,
                                               const vk::PhysicalDevice& vk_physical_device = {}, const vk::SurfaceKHR& vk_present_surface = {})
{
    META_FUNCTION_TASK();

    // Try to find queue family with exact flags match
    const std::optional<uint32_t> vk_exact_family_index = FindQueueFamily<true>(vk_queue_family_properties, queue_flags, queues_count, reserved_queues_count_per_family, vk_physical_device, vk_present_surface);
    if (vk_exact_family_index)
        return *vk_exact_family_index;

    // If no family with exact match, find one which contains a subset of queue flags
    return FindQueueFamily<false>(vk_queue_family_properties, queue_flags, queues_count, reserved_queues_count_per_family, vk_physical_device, vk_present_surface);
}

static bool AddDeviceQueueCreateInfo(const std::vector<vk::QueueFamilyProperties> vk_queue_family_properties,
                                     vk::QueueFlagBits queue_flags, uint32_t queues_count,
                                     std::vector<uint32_t>& reserved_queues_count_per_family,
                                     std::map<uint32_t, std::vector<float>>& vk_queue_priorities_per_family,
                                     std::vector<vk::DeviceQueueCreateInfo>& vk_queue_create_infos,
                                     const vk::PhysicalDevice& vk_physical_device = {}, const vk::SurfaceKHR& vk_present_surface = {})
{
    META_FUNCTION_TASK();
    const std::optional<uint32_t> vk_queue_family_index = FindQueueFamily(vk_queue_family_properties, queue_flags, queues_count, reserved_queues_count_per_family,
                                                                          vk_physical_device, vk_present_surface);
    if (!vk_queue_family_index)
        return false;

    uint32_t& reserved_queues_count = reserved_queues_count_per_family.at(*vk_queue_family_index);
    reserved_queues_count += queues_count;

    std::vector<float>& vk_queue_priorities = vk_queue_priorities_per_family[*vk_queue_family_index];
    vk_queue_priorities.resize(reserved_queues_count, 0.F);

    const auto vk_queue_create_info_it = std::find_if(vk_queue_create_infos.begin(), vk_queue_create_infos.end(),
                                                      [&vk_queue_family_index](const vk::DeviceQueueCreateInfo& create_info)
                                                      { return create_info.queueFamilyIndex == *vk_queue_family_index; }
    );

    if (vk_queue_create_info_it != vk_queue_create_infos.end())
    {
        vk_queue_create_info_it->queueCount = reserved_queues_count;
        vk_queue_create_info_it->pQueuePriorities = vk_queue_priorities.data();
        return true;
    }

    vk_queue_create_infos.emplace_back(vk::DeviceQueueCreateFlags(), *vk_queue_family_index, queues_count, vk_queue_priorities.data());
    return true;
}

static bool IsSoftwarePhysicalDevice(const vk::PhysicalDevice& vk_physical_device)
{
    META_FUNCTION_TASK();
    const vk::PhysicalDeviceType vk_device_type = vk_physical_device.getProperties().deviceType;
    return vk_device_type == vk::PhysicalDeviceType::eVirtualGpu ||
           vk_device_type == vk::PhysicalDeviceType::eCpu;
}

Device::Features DeviceVK::GetSupportedFeatures(const vk::PhysicalDevice&)
{
    META_FUNCTION_TASK();
    return Device::Features::BasicRendering;
}

DeviceVK::DeviceVK(const vk::PhysicalDevice& vk_physical_device, const std::vector<vk::DeviceQueueCreateInfo>& vk_queue_create_infos)
    : DeviceBase(vk_physical_device.getProperties().deviceName,
                 IsSoftwarePhysicalDevice(vk_physical_device),
                 Device::Features::BasicRendering)
    , m_vk_physical_device(vk_physical_device)
    , m_vk_device(vk_physical_device.createDevice(vk::DeviceCreateInfo(vk::DeviceCreateFlags(), vk_queue_create_infos)))
{
    META_FUNCTION_TASK();
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_vk_device);
}

DeviceVK::~DeviceVK()
{
    META_FUNCTION_TASK();
    m_vk_device.destroy();
}

System& System::Get()
{
    META_FUNCTION_TASK();
    static SystemVK s_system;
    return s_system;
}

SystemVK::SystemVK()
    : m_vk_instance(CreateVulkanInstance(m_vk_loader, {}, GetVulkanInstanceRequiredExtensions(), VK_API_VERSION_1_1))
{
    META_FUNCTION_TASK();
}

SystemVK::~SystemVK()
{
    META_FUNCTION_TASK();
    m_vk_instance.destroy();
    if (m_vk_surface)
    {
        m_vk_instance.destroySurfaceKHR(m_vk_surface);
    }
}

void SystemVK::CheckForChanges()
{
    META_FUNCTION_TASK();
    META_FUNCTION_NOT_IMPLEMENTED();
}

const Ptrs<Device>& SystemVK::UpdateGpuDevices(const Platform::AppEnvironment& app_env, const Device::Capabilities& required_device_caps)
{
    META_FUNCTION_TASK();
    META_UNUSED(app_env);
    if (required_device_caps.present_to_window && !m_vk_surface)
    {
        m_vk_surface = CreateVulkanSurfaceForWindow(m_vk_instance, app_env);
    }
    return UpdateGpuDevices(required_device_caps);
}

const Ptrs<Device>& SystemVK::UpdateGpuDevices(const Device::Capabilities& required_device_caps)
{
    META_FUNCTION_TASK();
    SetDeviceCapabilities(required_device_caps);
    ClearDevices();

    const std::vector<vk::PhysicalDevice> vk_physical_devices = m_vk_instance.enumeratePhysicalDevices();
    for(const vk::PhysicalDevice& vk_physical_device : vk_physical_devices)
    {
        AddDevice(vk_physical_device);
    }

    return GetGpuDevices();
}

void SystemVK::AddDevice(const vk::PhysicalDevice& vk_physical_device)
{
    META_FUNCTION_TASK();
    const Device::Capabilities& device_caps = GetDeviceCapabilities();
    const Device::Features device_supported_features = DeviceVK::GetSupportedFeatures(vk_physical_device);

    using namespace magic_enum::bitwise_operators;
    if (!magic_enum::flags::enum_contains(device_supported_features & GetDeviceCapabilities().features))
        return;

    const std::vector<vk::QueueFamilyProperties> vk_queue_family_properties = vk_physical_device.getQueueFamilyProperties();
    std::vector<uint32_t> reserved_queues_count_per_family(vk_queue_family_properties.size(), 0U);
    std::vector<vk::DeviceQueueCreateInfo> vk_queue_create_infos;
    std::map<uint32_t, std::vector<float>> queue_priorities_per_family;

    if (device_caps.render_queues_count)
    {
        if (device_caps.present_to_window &&
            !AddDeviceQueueCreateInfo(vk_queue_family_properties, vk::QueueFlagBits::eGraphics, 1U,
                                      reserved_queues_count_per_family, queue_priorities_per_family, vk_queue_create_infos,
                                      vk_physical_device, m_vk_surface))
            return;

        if ((!device_caps.present_to_window || device_caps.render_queues_count > 1) &&
            !AddDeviceQueueCreateInfo(vk_queue_family_properties, vk::QueueFlagBits::eGraphics,
                                      device_caps.render_queues_count - (device_caps.present_to_window ? 1U : 0U),
                                      reserved_queues_count_per_family, queue_priorities_per_family, vk_queue_create_infos))
            return;

    }

    if (device_caps.compute_queues_count &&
        !AddDeviceQueueCreateInfo(vk_queue_family_properties, vk::QueueFlagBits::eCompute, device_caps.compute_queues_count,
                                  reserved_queues_count_per_family, queue_priorities_per_family, vk_queue_create_infos))
        return;

    if (device_caps.blit_queues_count &&
        !AddDeviceQueueCreateInfo(vk_queue_family_properties, vk::QueueFlagBits::eTransfer, device_caps.blit_queues_count,
                                  reserved_queues_count_per_family, queue_priorities_per_family, vk_queue_create_infos))
        return;

    SystemBase::AddDevice(std::make_shared<DeviceVK>(vk_physical_device, vk_queue_create_infos));
}

} // namespace Methane::Graphics
