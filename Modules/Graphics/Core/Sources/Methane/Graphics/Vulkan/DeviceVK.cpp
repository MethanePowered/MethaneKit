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

FILE: Methane/Graphics/Vulkan/DeviceVK.cpp
Vulkan implementation of the device interface.

******************************************************************************/

#include "DeviceVK.h"
#include "PlatformVK.h"
#include "UtilsVK.hpp"

#include <Methane/Graphics/TypeFormatters.hpp>
#include <Methane/Platform/Utils.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <fmt/format.h>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <string_view>

//#define VULKAN_VALIDATION_BEST_PRACTICES_ENABLED

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace Methane::Graphics
{

static const std::string g_vk_app_name    = "Methane Powered App";
static const std::string g_vk_engine_name = "Methane Kit";

static const std::string g_vk_validation_layer        = "VK_LAYER_KHRONOS_validation";
static const std::string g_vk_debug_utils_extension   = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
static const std::string g_vk_validation_extension    = VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME;

// Google extensions are used to reflect HLSL semantic names from shader input decorations,
// and it works fine, but is not listed by Windows NVidia drivers, who knows why?
#ifdef __linux__
#define VK_GOOGLE_SPIRV_EXTENSIONS_ENABLED
#endif

static const std::vector<std::string_view> g_common_device_extensions{
    VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
#ifdef METHANE_GPU_INSTRUMENTATION_ENABLED
    VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME,
#endif
#ifndef __APPLE__
    VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
#endif
#ifdef VK_GOOGLE_SPIRV_EXTENSIONS_ENABLED
    VK_GOOGLE_HLSL_FUNCTIONALITY1_EXTENSION_NAME,
    VK_GOOGLE_USER_TYPE_EXTENSION_NAME,
#endif
};

static const std::vector<std::string_view> g_render_device_extensions{
    VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME
};

static const std::vector<std::string_view> g_present_device_extensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

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

    Platform::PrintToDebugOutput(ss.str());
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

template<bool exact_flags_matching>
std::optional<uint32_t> FindQueueFamily(const std::vector<vk::QueueFamilyProperties>& vk_queue_family_properties,
                                        vk::QueueFlags queue_flags, uint32_t queues_count,
                                        const std::vector<uint32_t>& reserved_queues_count_per_family,
                                        const vk::PhysicalDevice& vk_physical_device = {},
                                        const vk::SurfaceKHR& vk_present_surface = {})
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL(reserved_queues_count_per_family.size(), vk_queue_family_properties.size());
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
            if ((vk_family_props.queueFlags & queue_flags) != queue_flags)
                continue;
        }

        if (vk_family_props.queueCount < reserved_queues_count_per_family[family_index] + queues_count)
            continue;

        if (queue_flags == vk::QueueFlagBits::eGraphics && vk_physical_device && vk_present_surface &&
            !vk_physical_device.getSurfaceSupportKHR(static_cast<uint32_t>(family_index), vk_present_surface))
            continue;

#ifdef METHANE_GPU_INSTRUMENTATION_ENABLED
        if (!vk_family_props.timestampValidBits)
            continue;
#endif

        return static_cast<uint32_t>(family_index);
    }

    return {};
}

static std::optional<uint32_t> FindQueueFamily(const std::vector<vk::QueueFamilyProperties>& vk_queue_family_properties,
                                               vk::QueueFlags queue_flags, uint32_t queues_count,
                                               const std::vector<uint32_t>& reserved_queues_count_per_family,
                                               const vk::PhysicalDevice& vk_physical_device = {},
                                               const vk::SurfaceKHR& vk_present_surface = {})
{
    META_FUNCTION_TASK();

    // Try to find queue family with exact flags match
    if (const std::optional<uint32_t> family_index = FindQueueFamily<true>(vk_queue_family_properties, queue_flags, queues_count,
                                                                           reserved_queues_count_per_family, vk_physical_device, vk_present_surface);
        family_index) // NOSONAR - can not use value_or here
        return *family_index;

    // If no family with exact match, find one which contains a subset of queue flags
    return FindQueueFamily<false>(vk_queue_family_properties, queue_flags, queues_count,
                                  reserved_queues_count_per_family, vk_physical_device, vk_present_surface);
}

static bool IsSoftwarePhysicalDevice(const vk::PhysicalDevice& vk_physical_device)
{
    META_FUNCTION_TASK();
    const vk::PhysicalDeviceType vk_device_type = vk_physical_device.getProperties().deviceType;
    return vk_device_type == vk::PhysicalDeviceType::eVirtualGpu ||
           vk_device_type == vk::PhysicalDeviceType::eCpu;
}

static vk::QueueFlags GetQueueFlagsByType(CommandList::Type cmd_list_type)
{
    META_FUNCTION_TASK();
    switch(cmd_list_type)
    {
    case CommandList::Type::Blit:   return vk::QueueFlagBits::eTransfer;
    case CommandList::Type::Render: return vk::QueueFlagBits::eGraphics;
    default: META_UNEXPECTED_ARG_RETURN(cmd_list_type, vk::QueueFlagBits::eGraphics);
    }
}

QueueFamilyReservationVK::QueueFamilyReservationVK(uint32_t family_index, vk::QueueFlags queue_flags, uint32_t queues_count, bool can_present_to_window)
    : m_family_index(family_index)
    , m_queue_flags(queue_flags)
    , m_queues_count(queues_count)
    , m_can_present_to_window(can_present_to_window)
    , m_priorities(m_queues_count, 0.F)
    , m_free_indices({ { 0U, m_queues_count } })
{
    META_FUNCTION_TASK();
}

QueueFamilyReservationVK::~QueueFamilyReservationVK()
{
    META_FUNCTION_TASK();

    // All command queues must be released upon device destruction
    assert(m_free_indices == Data::RangeSet<uint32_t>({ { 0U, m_queues_count } }));
}

vk::DeviceQueueCreateInfo QueueFamilyReservationVK::MakeDeviceQueueCreateInfo() const noexcept
{
    META_FUNCTION_TASK();
    return vk::DeviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), m_family_index, m_queues_count, m_priorities.data());
}

uint32_t QueueFamilyReservationVK::ClaimQueueIndex() const
{
    META_FUNCTION_TASK();
    if (m_free_indices.IsEmpty())
        throw EmptyArgumentException<Data::RangeSet<uint32_t>>(__FUNCTION_NAME__, "m_free_indices", "device queue family has no free queues in reservation");

    const uint32_t free_queue_index = m_free_indices.begin()->GetStart();
    m_free_indices.Remove({ free_queue_index, free_queue_index + 1});
    return free_queue_index;
}

void QueueFamilyReservationVK::ReleaseQueueIndex(uint32_t queue_index) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS(queue_index, m_queues_count);
    m_free_indices.Add({ queue_index, queue_index + 1});
}

void QueueFamilyReservationVK::IncrementQueuesCount(uint32_t extra_queues_count) noexcept
{
    META_FUNCTION_TASK();
    if (!extra_queues_count)
        return;

    m_free_indices.Add({m_queues_count, m_queues_count + extra_queues_count});
    m_queues_count += extra_queues_count;
    m_priorities.resize(m_queues_count, 0.F);
}

Device::Features DeviceVK::GetSupportedFeatures(const vk::PhysicalDevice& vk_physical_device)
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;
    vk::PhysicalDeviceFeatures vk_device_features = vk_physical_device.getFeatures();
    Device::Features device_features = Device::Features::BasicRendering;
    if (vk_device_features.samplerAnisotropy)
        device_features |= Device::Features::AnisotropicFiltering;
    if (vk_device_features.imageCubeArray)
        device_features |= Device::Features::ImageCubeArray;
    return device_features;
}

DeviceVK::DeviceVK(const vk::PhysicalDevice& vk_physical_device, const vk::SurfaceKHR& vk_surface, const Capabilities& capabilities)
    : DeviceBase(vk_physical_device.getProperties().deviceName,
                 IsSoftwarePhysicalDevice(vk_physical_device),
                 capabilities)
    , m_vk_physical_device(vk_physical_device)
    , m_vk_queue_family_properties(vk_physical_device.getQueueFamilyProperties())
{
    META_FUNCTION_TASK();

    using namespace magic_enum::bitwise_operators;
    if (const Device::Features device_supported_features = DeviceVK::GetSupportedFeatures(vk_physical_device);
        !static_cast<bool>(device_supported_features & capabilities.features))
        throw IncompatibleException("Supported Device features are incompatible with the required capabilities");

    std::vector<uint32_t> reserved_queues_count_per_family(m_vk_queue_family_properties.size(), 0U);

    if (capabilities.present_to_window && !IsExtensionSupported(g_present_device_extensions))
        throw IncompatibleException("Device does not support some of required extensions");

    ReserveQueueFamily(CommandList::Type::Render, capabilities.render_queues_count, reserved_queues_count_per_family,
                       capabilities.present_to_window ? vk_surface : vk::SurfaceKHR());

    ReserveQueueFamily(CommandList::Type::Blit, capabilities.blit_queues_count, reserved_queues_count_per_family);

    std::vector<vk::DeviceQueueCreateInfo> vk_queue_create_infos;
    std::set<QueueFamilyReservationVK*> unique_family_reservation_ptrs;
    for(const auto& [type, queue_family_reservation_ptr] : m_queue_family_reservation_by_type)
    {
        if (!queue_family_reservation_ptr || unique_family_reservation_ptrs.count(queue_family_reservation_ptr.get()))
            continue;

        vk_queue_create_infos.emplace_back(queue_family_reservation_ptr->MakeDeviceQueueCreateInfo());
        unique_family_reservation_ptrs.insert(queue_family_reservation_ptr.get());
    }

    std::vector<std::string_view> enabled_extension_names = g_common_device_extensions;
    if (capabilities.present_to_window)
    {
        enabled_extension_names.insert(enabled_extension_names.end(), g_present_device_extensions.begin(), g_present_device_extensions.end());
    }
    if (static_cast<bool>(capabilities.features & Device::Features::BasicRendering))
    {
        enabled_extension_names.insert(enabled_extension_names.end(), g_render_device_extensions.begin(), g_render_device_extensions.end());
    }

    std::vector<const char*> raw_enabled_extension_names;
    std::transform(enabled_extension_names.begin(), enabled_extension_names.end(), std::back_inserter(raw_enabled_extension_names),
                   [](const std::string_view& extension_name) { return extension_name.data(); });

    // Enable physical device features:
    vk::PhysicalDeviceFeatures vk_device_features;
    vk_device_features.samplerAnisotropy = static_cast<bool>(capabilities.features & Features::AnisotropicFiltering);
    vk_device_features.imageCubeArray    = static_cast<bool>(capabilities.features & Features::ImageCubeArray);

    // Add descriptions of enabled device features:
    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT vk_device_dynamic_state_feature(true);
    vk::PhysicalDeviceTimelineSemaphoreFeaturesKHR    vk_device_timeline_semaphores_feature(true);
    vk::PhysicalDeviceHostQueryResetFeatures          vk_device_host_query_reset_feature(true);
    vk::DeviceCreateInfo vk_device_info(vk::DeviceCreateFlags{}, vk_queue_create_infos, { }, raw_enabled_extension_names, &vk_device_features);
    vk_device_info.setPNext(&vk_device_dynamic_state_feature);
    vk_device_dynamic_state_feature.setPNext(&vk_device_timeline_semaphores_feature);
    vk_device_timeline_semaphores_feature.setPNext(&vk_device_host_query_reset_feature);

#ifndef __APPLE__
    vk::PhysicalDeviceSynchronization2FeaturesKHR vk_device_synchronization_2_feature(true);
    vk_device_host_query_reset_feature.setPNext(&vk_device_synchronization_2_feature);
#endif

    m_vk_unique_device = vk_physical_device.createDeviceUnique(vk_device_info);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_vk_unique_device.get());
}

bool DeviceVK::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (!DeviceBase::SetName(name))
        return false;

    SetVulkanObjectName(m_vk_unique_device.get(), m_vk_unique_device.get(), name.c_str());
    return true;
}

bool DeviceVK::IsExtensionSupported(const std::vector<std::string_view>& required_extensions) const
{
    META_FUNCTION_TASK();
    std::set<std::string_view> extensions(required_extensions.begin(), required_extensions.end());
    const std::vector<vk::ExtensionProperties> vk_device_extension_properties = m_vk_physical_device.enumerateDeviceExtensionProperties();
    for(const vk::ExtensionProperties& vk_extension_props : vk_device_extension_properties)
    {
        extensions.erase(vk_extension_props.extensionName);
    }
    return extensions.empty();
}

const QueueFamilyReservationVK* DeviceVK::GetQueueFamilyReservationPtr(CommandList::Type cmd_list_type) const noexcept
{
    META_FUNCTION_TASK();
    const auto queue_family_reservation_by_type_it = m_queue_family_reservation_by_type.find(cmd_list_type);
    return queue_family_reservation_by_type_it != m_queue_family_reservation_by_type.end()
         ? queue_family_reservation_by_type_it->second.get()
         : nullptr;
}

const QueueFamilyReservationVK& DeviceVK::GetQueueFamilyReservation(CommandList::Type cmd_list_type) const
{
    META_FUNCTION_TASK();
    const QueueFamilyReservationVK* queue_family_reservation_ptr = GetQueueFamilyReservationPtr(cmd_list_type);
    META_CHECK_ARG_NOT_NULL_DESCR(queue_family_reservation_ptr, fmt::format("queue family was not reserved for {} command list type", cmd_list_type));
    return *queue_family_reservation_ptr;
}

DeviceVK::SwapChainSupport DeviceVK::GetSwapChainSupportForSurface(const vk::SurfaceKHR& vk_surface) const noexcept
{
    META_FUNCTION_TASK();
    return SwapChainSupport{
        m_vk_physical_device.getSurfaceCapabilitiesKHR(vk_surface),
        m_vk_physical_device.getSurfaceFormatsKHR(vk_surface),
        m_vk_physical_device.getSurfacePresentModesKHR(vk_surface)
    };
}

Opt<uint32_t> DeviceVK::FindMemoryType(uint32_t type_filter, vk::MemoryPropertyFlags property_flags) const noexcept
{
    META_FUNCTION_TASK();
    const vk::PhysicalDeviceMemoryProperties vk_memory_props = m_vk_physical_device.getMemoryProperties();
    for(uint32_t type_index = 0U; type_index < vk_memory_props.memoryTypeCount; ++type_index)
    {
        if (type_filter & (1 << type_index) &&
            (vk_memory_props.memoryTypes[type_index].propertyFlags & property_flags) == property_flags)
            return type_index;
    }
    return std::nullopt;
}

const vk::QueueFamilyProperties& DeviceVK::GetNativeQueueFamilyProperties(uint32_t queue_family_index) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS_DESCR(queue_family_index, m_vk_queue_family_properties.size(), "invalid queue family index");
    return m_vk_queue_family_properties[queue_family_index];
}

void DeviceVK::ReserveQueueFamily(CommandList::Type cmd_list_type, uint32_t queues_count,
                                  std::vector<uint32_t>& reserved_queues_count_per_family,
                                  const vk::SurfaceKHR& vk_surface)
{
    META_FUNCTION_TASK();
    if (!queues_count)
        return;

    const vk::QueueFlags queue_flags = GetQueueFlagsByType(cmd_list_type);
    const std::optional<uint32_t> vk_queue_family_index = FindQueueFamily(m_vk_queue_family_properties, queue_flags, queues_count,
                                                                          reserved_queues_count_per_family, m_vk_physical_device, vk_surface);
    if (!vk_queue_family_index)
        throw IncompatibleException(fmt::format("Device does not support the required queue type {} and count {}", magic_enum::enum_name(cmd_list_type), queues_count));

    META_CHECK_ARG_LESS(*vk_queue_family_index, m_vk_queue_family_properties.size());
    META_CHECK_ARG_TRUE(static_cast<bool>(m_vk_queue_family_properties[*vk_queue_family_index].queueFlags & queue_flags));
    const auto queue_family_reservation_it = std::find_if(m_queue_family_reservation_by_type.begin(), m_queue_family_reservation_by_type.end(),
                                                          [&vk_queue_family_index](const auto& type_and_queue_family_reservation)
                                                          { return type_and_queue_family_reservation.second->GetFamilyIndex() == *vk_queue_family_index; });

    const bool is_new_queue_family_reservation = queue_family_reservation_it == m_queue_family_reservation_by_type.end();
    if (!is_new_queue_family_reservation && queue_family_reservation_it->second)
        queue_family_reservation_it->second->IncrementQueuesCount(queues_count);

    reserved_queues_count_per_family[*vk_queue_family_index] += queues_count;
    m_queue_family_reservation_by_type.try_emplace(cmd_list_type,
        is_new_queue_family_reservation
            ? std::make_shared<QueueFamilyReservationVK>(*vk_queue_family_index, queue_flags, queues_count, static_cast<bool>(vk_surface))
            : queue_family_reservation_it->second
    );

    META_LOG("Vulkan command queue family [{}] was reserved for allocating {} {} queues.",
             *vk_queue_family_index, queues_count, magic_enum::enum_name(cmd_list_type));
}

System& System::Get()
{
    META_FUNCTION_TASK();
    static const auto s_system_ptr = std::make_shared<SystemVK>();
    return *s_system_ptr;
}

SystemVK::SystemVK()
    : m_vk_unique_instance(CreateVulkanInstance(m_vk_loader, {}, PlatformVK::GetVulkanInstanceRequiredExtensions(), VK_API_VERSION_1_1))
#ifndef NDEBUG
    , m_vk_unique_debug_utils_messanger(m_vk_unique_instance.get().createDebugUtilsMessengerEXTUnique(MakeDebugUtilsMessengerCreateInfoEXT()))
#endif
{
    META_FUNCTION_TASK();
}

SystemVK::~SystemVK()
{
    META_FUNCTION_TASK();

    // Devices have to be destroyed strictly before Vulkan instance
    ClearDevices();
}

void SystemVK::CheckForChanges()
{
    META_FUNCTION_TASK();
}

const Ptrs<Device>& SystemVK::UpdateGpuDevices(const Platform::AppEnvironment& app_env, const Device::Capabilities& required_device_caps)
{
    META_FUNCTION_TASK();
    if (required_device_caps.present_to_window && !m_vk_unique_surface)
    {
        // Temporary surface object is used only to test devices for ability to present to the window
        m_vk_unique_surface = PlatformVK::CreateVulkanSurfaceForWindow(GetNativeInstance(), app_env);
    }

    const Ptrs<Device>& gpu_devices = UpdateGpuDevices(required_device_caps);

    if (m_vk_unique_surface)
    {
        // When devices are created, temporary surface can be released
        m_vk_unique_surface.release();
    }
    return gpu_devices;
}

const Ptrs<Device>& SystemVK::UpdateGpuDevices(const Device::Capabilities& required_device_caps)
{
    META_FUNCTION_TASK();
    SetDeviceCapabilities(required_device_caps);
    ClearDevices();

    const std::vector<vk::PhysicalDevice> vk_physical_devices = GetNativeInstance().enumeratePhysicalDevices();
    for(const vk::PhysicalDevice& vk_physical_device : vk_physical_devices)
    {
        try
        {
            AddDevice(std::make_shared<DeviceVK>(vk_physical_device, m_vk_unique_surface.get(), required_device_caps));
        }
        catch(const DeviceVK::IncompatibleException& ex)
        {
            META_LOG("Physical GPU device is skipped: {}", ex.what());
            META_UNUSED(ex);
            continue;
        }
    }

    return GetGpuDevices();
}

} // namespace Methane::Graphics
