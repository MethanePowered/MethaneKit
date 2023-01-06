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

FILE: Methane/Graphics/Vulkan/System.cpp
Vulkan implementation of the system interface.

******************************************************************************/

#include <Methane/Graphics/Vulkan/System.h>
#include <Methane/Graphics/Vulkan/Device.h>
#include <Methane/Graphics/Vulkan/Platform.h>
#include <Methane/Graphics/Vulkan/Utils.hpp>

#include <Methane/Graphics/TypeFormatters.hpp>
#include <Methane/Platform/Utils.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <fmt/format.h>
#include <magic_enum.hpp>

#include <vector>
#include <sstream>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <string_view>

//#define VULKAN_VALIDATION_BEST_PRACTICES_ENABLED

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace Methane::Graphics::Rhi
{

ISystem& ISystem::Get()
{
    META_FUNCTION_TASK();
    static const auto s_system_ptr = std::make_shared<Vulkan::System>();
    return *s_system_ptr;
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::Vulkan
{

static const std::string g_vk_app_name    = "Methane Powered App";
static const std::string g_vk_engine_name = "Methane Kit";

static const std::string g_vk_validation_layer        = "VK_LAYER_KHRONOS_validation";
static const std::string g_vk_debug_utils_extension   = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
static const std::string g_vk_validation_extension    = VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME;

static std::vector<char const*> GetEnabledLayers(const std::vector<std::string_view>& layers)
{
    META_FUNCTION_TASK();
    const std::vector<vk::LayerProperties> layer_properties = vk::enumerateInstanceLayerProperties();

    std::vector<const char*> enabled_layers;
    enabled_layers.reserve(layers.size() );
    for (const std::string_view& layer : layers)
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

static std::vector<const char*> GetEnabledExtensions(const std::vector<std::string_view>& extensions)
{
    META_FUNCTION_TASK();

    const std::vector<vk::ExtensionProperties>& extension_properties = vk::enumerateInstanceExtensionProperties();
    std::vector<const char*> enabled_extensions;
    enabled_extensions.reserve(extensions.size());

    for (const std::string_view& ext : extensions)
    {
        assert(std::find_if(extension_properties.begin(), extension_properties.end(),
                            [ext](const vk::ExtensionProperties& ep) { return ext == ep.extensionName; }
                            ) != extension_properties.end());
        enabled_extensions.push_back(ext.data() );
    }

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
    META_UNUSED(add_enabled_extension);

#ifndef NDEBUG
    add_enabled_extension(g_vk_debug_utils_extension);
#endif

#if defined(VULKAN_VALIDATION_BEST_PRACTICES_ENABLED) && !defined(NDEBUG)
    add_enabled_extension(g_vk_validation_extension);
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

    // Assert on calling vkBeginCommandBuffer() on active VkCommandBuffer before it has completed. You must check command buffer fence before this call.
    //assert(callback_data_ptr->messageIdNumber == -2080204129); // VUID-vkBeginCommandBuffer-commandBuffer-00049

    if (callback_data_ptr->messageIdNumber == 648835635 || // UNASSIGNED-khronos-Validation-debug-build-warning-message
        callback_data_ptr->messageIdNumber == 767975156 || // UNASSIGNED-BestPractices-vkCreateInstance-specialise-extension
        callback_data_ptr->messageIdNumber == -400166253)  // UNASSIGNED-CoreValidation-DrawState-QueueForwardProgress
        return VK_FALSE;

    if (callback_data_ptr->messageIdNumber == 0 && (
        strstr(callback_data_ptr->pMessage, "loader_get_json: Failed to open JSON file") ||
        strstr(callback_data_ptr->pMessage, "terminator_CreateInstance: Failed to CreateInstance in ICD")))
        return VK_FALSE;

#ifndef VK_GOOGLE_SPIRV_EXTENSIONS_ENABLED
    // Filter out validation error appeared due to missing HLSL extension for SPIRV bytecode, which can not be used because of bug in NVidia Windows drivers:
    // vkCreateShaderModule(): The SPIR-V Extension (SPV_GOOGLE_hlsl_functionality1 | SPV_GOOGLE_user_type) was declared, but none of the requirements were met to use it.
    if (callback_data_ptr->messageIdNumber == 1028204675 && // VUID-VkShaderModuleCreateInfo-pCode-04147
        strstr(callback_data_ptr->pMessage, "SPV_GOOGLE_"))
        return VK_FALSE;
#endif // VK_GOOGLE_SPIRV_EXTENSIONS_ENABLED

#endif // !NDEBUG

    std::stringstream ss;
    ss << vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(message_severity)) << " "
       << vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(message_types)) << ":" << std::endl;
    ss << "\t- messageIdName:   " << callback_data_ptr->pMessageIdName << std::endl;
    ss << "\t- messageIdNumber: " << callback_data_ptr->messageIdNumber << std::endl;
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
            ss << "\t\t\t- objectType:   " << vk::to_string( static_cast<vk::ObjectType>(callback_data_ptr->pObjects[i].objectType)) << std::endl;
            ss << "\t\t\t- objectHandle: " << callback_data_ptr->pObjects[i].objectHandle << std::endl;
            if (callback_data_ptr->pObjects[i].pObjectName)
                ss << "\t\t\t- objectName:   " << callback_data_ptr->pObjects[i].pObjectName << std::endl;
        }
    }

    Methane::Platform::PrintToDebugOutput(ss.str());
    return VK_FALSE;
}

#ifndef NDEBUG
static vk::DebugUtilsMessengerCreateInfoEXT MakeDebugUtilsMessengerCreateInfoEXT()
{
    return vk::DebugUtilsMessengerCreateInfoEXT {
        vk::DebugUtilsMessengerCreateFlagsEXT{},
        vk::DebugUtilsMessageSeverityFlagsEXT {
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
        },
        vk::DebugUtilsMessageTypeFlagsEXT {
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
        },
        &DebugUtilsMessengerCallback
    };
}
#endif // !NDEBUG

#ifdef NDEBUG
using InstanceCreateInfoChain = vk::StructureChain<vk::InstanceCreateInfo>;
#elif defined(VULKAN_VALIDATION_BEST_PRACTICES_ENABLED)
using InstanceCreateInfoChain = vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT, vk::ValidationFeaturesEXT>;
#else
using InstanceCreateInfoChain = vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT>;
#endif

static InstanceCreateInfoChain MakeInstanceCreateInfoChain(const vk::ApplicationInfo& vk_app_info,
                                                           vk::InstanceCreateFlags vk_instance_create_flags,
                                                           const std::vector<const char*>& layers,
                                                           const std::vector<const char*>& extensions)
{
    META_FUNCTION_TASK();

#ifdef NDEBUG
    return InstanceCreateInfoChain({ vk_instance_create_flags, &vk_app_info, layers, extensions });
#else   
    return InstanceCreateInfoChain(
        { vk_instance_create_flags, &vk_app_info, layers, extensions },
        MakeDebugUtilsMessengerCreateInfoEXT()
#ifdef VULKAN_VALIDATION_BEST_PRACTICES_ENABLED
        , { vk::ValidationFeatureEnableEXT::eBestPractices }
#endif
    );
#endif
}

static vk::UniqueInstance CreateVulkanInstance(const vk::DynamicLoader& vk_loader,
                                               const std::vector<std::string_view>& layers = {},
                                               const std::vector<std::string_view>& extensions = {},
                                               uint32_t vk_api_version = VK_API_VERSION_1_1)
{
    META_FUNCTION_TASK();

    VULKAN_HPP_DEFAULT_DISPATCHER.init(vk_loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));

    vk::InstanceCreateFlags vk_instance_create_flags{};
    if (std::find_if(extensions.begin(), extensions.end(),
                     [](const std::string_view& ext) { return ext == VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME; })
                     != extensions.end())
        vk_instance_create_flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;

    constexpr uint32_t engine_version = METHANE_VERSION_MAJOR * 10 + METHANE_VERSION_MINOR;
    const std::vector<const char*> enabled_layers     = GetEnabledLayers(layers);
    const std::vector<const char*> enabled_extensions = GetEnabledExtensions(extensions);
    const vk::ApplicationInfo vk_app_info(g_vk_app_name.c_str(), 1, g_vk_engine_name.c_str(), engine_version, vk_api_version);
    const vk::InstanceCreateInfo vk_instance_create_info = MakeInstanceCreateInfoChain(vk_app_info, vk_instance_create_flags,
                                                                                       enabled_layers, enabled_extensions).get<vk::InstanceCreateInfo>();

    vk::UniqueInstance vk_unique_instance = vk::createInstanceUnique(vk_instance_create_info);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vk_unique_instance.get());
    return vk_unique_instance;
}

System::System()
    : m_vk_unique_instance(CreateVulkanInstance(m_vk_loader, {}, Platform::GetVulkanInstanceRequiredExtensions(), VK_API_VERSION_1_1))
#ifndef NDEBUG
    , m_vk_unique_debug_utils_messanger(m_vk_unique_instance.get().createDebugUtilsMessengerEXTUnique(MakeDebugUtilsMessengerCreateInfoEXT()))
#endif
{
    META_FUNCTION_TASK();
}

System::~System()
{
    META_FUNCTION_TASK();

    // Devices have to be destroyed strictly before Vulkan instance
    ClearDevices();
}

void System::CheckForChanges()
{
    META_FUNCTION_TASK();
}

const Ptrs<Rhi::IDevice>& System::UpdateGpuDevices(const Methane::Platform::AppEnvironment& app_env, const Rhi::DeviceCaps& required_device_caps)
{
    META_FUNCTION_TASK();
    if (required_device_caps.features.HasAnyBit(Rhi::DeviceFeature::PresentToWindow) && !m_vk_unique_surface)
    {
        // Temporary surface object is used only to test devices for ability to present to the window
        m_vk_unique_surface = Platform::CreateVulkanSurfaceForWindow(GetNativeInstance(), app_env);
    }

    const Ptrs<Rhi::IDevice>& gpu_devices = UpdateGpuDevices(required_device_caps);

    if (m_vk_unique_surface)
    {
        // When devices are created, temporary surface can be released
        m_vk_unique_surface.release();
    }
    return gpu_devices;
}

const Ptrs<Rhi::IDevice>& System::UpdateGpuDevices(const Rhi::DeviceCaps& required_device_caps)
{
    META_FUNCTION_TASK();
    SetDeviceCapabilities(required_device_caps);
    ClearDevices();

    const std::vector<vk::PhysicalDevice> vk_physical_devices = GetNativeInstance().enumeratePhysicalDevices();
    for(const vk::PhysicalDevice& vk_physical_device : vk_physical_devices)
    {
        try
        {
            AddDevice(std::make_shared<Device>(vk_physical_device, m_vk_unique_surface.get(), required_device_caps));
        }
        catch(const Device::IncompatibleException& ex)
        {
            META_LOG("Physical GPU device is skipped: {}", ex.what());
            META_UNUSED(ex);
            continue;
        }
    }

    return GetGpuDevices();
}

} // namespace Methane::Graphics::Vulkan
