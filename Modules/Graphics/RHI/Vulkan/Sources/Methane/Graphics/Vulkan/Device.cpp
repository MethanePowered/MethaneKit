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

FILE: Methane/Graphics/Vulkan/Device.cpp
Vulkan implementation of the device interface.

******************************************************************************/

#include <Methane/Graphics/Vulkan/Device.h>
#include <Methane/Graphics/Vulkan/Platform.h>
#include <Methane/Graphics/Vulkan/RenderContext.h>
#include <Methane/Graphics/Vulkan/ComputeContext.h>
#include <Methane/Graphics/Vulkan/Utils.hpp>

#include <Methane/Graphics/TypeFormatters.hpp>
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

namespace Methane::Graphics::Vulkan
{

// Google extensions are used to reflect HLSL semantic names from shader input decorations,
// and it works fine, but is not listed by Windows NVidia drivers, who knows why?
//#ifdef __linux__
//#define VK_GOOGLE_SPIRV_EXTENSIONS_ENABLED
//#endif

#define VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME "VK_KHR_portability_subset"

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

static vk::QueueFlags GetQueueFlagsByType(Rhi::CommandListType cmd_list_type)
{
    META_FUNCTION_TASK();
    switch(cmd_list_type)
    {
    case Rhi::CommandListType::Transfer: return vk::QueueFlagBits::eTransfer;
    case Rhi::CommandListType::Render:   return vk::QueueFlagBits::eGraphics;
    case Rhi::CommandListType::Compute:  return vk::QueueFlagBits::eCompute;
    default: META_UNEXPECTED_ARG_RETURN(cmd_list_type, vk::QueueFlagBits::eGraphics);
    }
}

static std::vector<std::string> GetDeviceSupportedExtensionNames(const vk::PhysicalDevice& vk_physical_device)
{
    META_FUNCTION_TASK();
    std::vector<std::string> supported_extensions;
    const std::vector<vk::ExtensionProperties> vk_device_extension_properties = vk_physical_device.enumerateDeviceExtensionProperties();
    for(const vk::ExtensionProperties& vk_extension_props : vk_device_extension_properties)
    {
        supported_extensions.emplace_back(vk_extension_props.extensionName.operator std::string());
    }
    return supported_extensions;
}

QueueFamilyReservation::QueueFamilyReservation(uint32_t family_index, vk::QueueFlags queue_flags, uint32_t queues_count, bool can_present_to_window)
    : m_family_index(family_index)
    , m_queue_flags(queue_flags)
    , m_queues_count(queues_count)
    , m_can_present_to_window(can_present_to_window)
    , m_priorities(m_queues_count, 0.F)
    , m_free_indices({ { 0U, m_queues_count } })
{ }

QueueFamilyReservation::~QueueFamilyReservation()
{
    META_FUNCTION_TASK();

    // All command queues must be released upon device destruction
    assert(m_free_indices == Data::RangeSet<uint32_t>({ { 0U, m_queues_count } }));
}

vk::DeviceQueueCreateInfo QueueFamilyReservation::MakeDeviceQueueCreateInfo() const noexcept
{
    META_FUNCTION_TASK();
    return vk::DeviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), m_family_index, m_queues_count, m_priorities.data());
}

uint32_t QueueFamilyReservation::ClaimQueueIndex() const
{
    META_FUNCTION_TASK();
    if (m_free_indices.IsEmpty())
        throw EmptyArgumentException<Data::RangeSet<uint32_t>>(__FUNCTION_NAME__, "m_free_indices", "device queue family has no free queues in reservation");

    const uint32_t free_queue_index = m_free_indices.begin()->GetStart();
    m_free_indices.Remove({ free_queue_index, free_queue_index + 1});
    return free_queue_index;
}

void QueueFamilyReservation::ReleaseQueueIndex(uint32_t queue_index) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS(queue_index, m_queues_count);
    m_free_indices.Add({ queue_index, queue_index + 1});
}

void QueueFamilyReservation::IncrementQueuesCount(uint32_t extra_queues_count) noexcept
{
    META_FUNCTION_TASK();
    if (!extra_queues_count)
        return;

    m_free_indices.Add({m_queues_count, m_queues_count + extra_queues_count});
    m_queues_count += extra_queues_count;
    m_priorities.resize(m_queues_count, 0.F);
}

Device::Device(const vk::PhysicalDevice& vk_physical_device, const vk::SurfaceKHR& vk_surface, const Capabilities& capabilities)
    : Base::Device(vk_physical_device.getProperties().deviceName,
                 IsSoftwarePhysicalDevice(vk_physical_device),
                 capabilities)
    , m_vk_physical_device(vk_physical_device)
    , m_supported_extension_names_storage(GetDeviceSupportedExtensionNames(vk_physical_device))
    , m_supported_extension_names_set(m_supported_extension_names_storage.begin(), m_supported_extension_names_storage.end())
    , m_is_dynamic_state_supported(IsExtensionSupported(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME))
    , m_vk_queue_family_properties(vk_physical_device.getQueueFamilyProperties())
{
    META_FUNCTION_TASK();
    if (const Rhi::DeviceFeatureMask device_supported_features = GetSupportedFeatures();
        !device_supported_features.HasBits(capabilities.features))
        throw IncompatibleException("Supported Device features are incompatible with the required capabilities");

    std::vector<uint32_t> reserved_queues_count_per_family(m_vk_queue_family_properties.size(), 0U);

    ReserveQueueFamily(Rhi::CommandListType::Render,   capabilities.render_queues_count,   reserved_queues_count_per_family,
                       capabilities.features.HasBit(Rhi::DeviceFeature::PresentToWindow) ? vk_surface : vk::SurfaceKHR());
    ReserveQueueFamily(Rhi::CommandListType::Transfer, capabilities.transfer_queues_count, reserved_queues_count_per_family);
    ReserveQueueFamily(Rhi::CommandListType::Compute,  capabilities.compute_queues_count,  reserved_queues_count_per_family);

    std::vector<vk::DeviceQueueCreateInfo> vk_queue_create_infos;
    std::set<QueueFamilyReservation*> unique_family_reservation_ptrs;
    for(const auto& [type, queue_family_reservation_ptr] : m_queue_family_reservation_by_type)
    {
        if (!queue_family_reservation_ptr || unique_family_reservation_ptrs.count(queue_family_reservation_ptr.get()))
            continue;

        vk_queue_create_infos.emplace_back(queue_family_reservation_ptr->MakeDeviceQueueCreateInfo());
        unique_family_reservation_ptrs.insert(queue_family_reservation_ptr.get());
    }

    std::vector<std::string_view> enabled_extension_names = g_common_device_extensions;
    if (capabilities.render_queues_count)
    {
        if (capabilities.features.HasBit(Rhi::DeviceFeature::PresentToWindow))
        {
            enabled_extension_names.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        }
        if (m_is_dynamic_state_supported)
        {
            enabled_extension_names.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
        }
    }

    if (IsExtensionSupported(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
    {
        enabled_extension_names.emplace_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
    }

    std::vector<const char*> raw_enabled_extension_names;
    std::transform(enabled_extension_names.begin(), enabled_extension_names.end(), std::back_inserter(raw_enabled_extension_names),
                   [](const std::string_view& extension_name) { return extension_name.data(); });

    // Enable physical device features:
    vk::PhysicalDeviceFeatures vk_device_features;
    vk_device_features.samplerAnisotropy = capabilities.features.HasBit(Rhi::DeviceFeature::AnisotropicFiltering);
    vk_device_features.imageCubeArray    = capabilities.features.HasBit(Rhi::DeviceFeature::ImageCubeArray);

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

Ptr<Rhi::IRenderContext> Device::CreateRenderContext(const Methane::Platform::AppEnvironment& env, tf::Executor& parallel_executor, const Rhi::RenderContextSettings& settings)
{
    META_FUNCTION_TASK();
    const auto render_context_ptr = std::make_shared<Vulkan::RenderContext>(env, *this, parallel_executor, settings);
    render_context_ptr->Initialize(*this, true);
    return render_context_ptr;
}

Ptr<Rhi::IComputeContext> Device::CreateComputeContext(tf::Executor& parallel_executor, const Rhi::ComputeContextSettings& settings)
{
    META_FUNCTION_TASK();
    const auto compute_context_ptr = std::make_shared<Vulkan::ComputeContext>(*this, parallel_executor, settings);
    compute_context_ptr->Initialize(*this, true);
    return compute_context_ptr;
}

bool Device::SetName(std::string_view name)
{
    META_FUNCTION_TASK();
    if (!Base::Device::SetName(name))
        return false;

    SetVulkanObjectName(m_vk_unique_device.get(), m_vk_unique_device.get(), name);
    return true;
}

bool Device::IsExtensionSupported(std::string_view required_extension) const
{
    META_FUNCTION_TASK();
    return m_supported_extension_names_set.count(required_extension);
}

const QueueFamilyReservation* Device::GetQueueFamilyReservationPtr(Rhi::CommandListType cmd_list_type) const noexcept
{
    META_FUNCTION_TASK();
    const auto queue_family_reservation_by_type_it = m_queue_family_reservation_by_type.find(cmd_list_type);
    return queue_family_reservation_by_type_it != m_queue_family_reservation_by_type.end()
         ? queue_family_reservation_by_type_it->second.get()
         : nullptr;
}

const QueueFamilyReservation& Device::GetQueueFamilyReservation(Rhi::CommandListType cmd_list_type) const
{
    META_FUNCTION_TASK();
    const QueueFamilyReservation* queue_family_reservation_ptr = GetQueueFamilyReservationPtr(cmd_list_type);
    META_CHECK_ARG_NOT_NULL_DESCR(queue_family_reservation_ptr,
                                  fmt::format("queue family was not reserved for {} command list type",
                                              magic_enum::enum_name(cmd_list_type)));
    return *queue_family_reservation_ptr;
}

Device::SwapChainSupport Device::GetSwapChainSupportForSurface(const vk::SurfaceKHR& vk_surface) const noexcept
{
    META_FUNCTION_TASK();
    return SwapChainSupport{
        m_vk_physical_device.getSurfaceCapabilitiesKHR(vk_surface),
        m_vk_physical_device.getSurfaceFormatsKHR(vk_surface),
        m_vk_physical_device.getSurfacePresentModesKHR(vk_surface)
    };
}

Opt<uint32_t> Device::FindMemoryType(uint32_t type_filter, vk::MemoryPropertyFlags property_flags) const noexcept
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

const vk::QueueFamilyProperties& Device::GetNativeQueueFamilyProperties(uint32_t queue_family_index) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS_DESCR(queue_family_index, m_vk_queue_family_properties.size(), "invalid queue family index");
    return m_vk_queue_family_properties[queue_family_index];
}

void Device::ReserveQueueFamily(Rhi::CommandListType cmd_list_type, uint32_t queues_count,
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
        throw IncompatibleException(fmt::format("Device does not support the required queue type {} and count {}",
                                                magic_enum::enum_name(cmd_list_type), queues_count));

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
            ? std::make_shared<QueueFamilyReservation>(*vk_queue_family_index, queue_flags, queues_count, static_cast<bool>(vk_surface))
            : queue_family_reservation_it->second
    );

    META_LOG("Vulkan command queue family [{}] was reserved for allocating {} {} queues.",
             *vk_queue_family_index, queues_count, magic_enum::enum_name(cmd_list_type));
}

Rhi::DeviceFeatureMask Device::GetSupportedFeatures() const
{
    META_FUNCTION_TASK();
    vk::PhysicalDeviceFeatures vk_device_features = m_vk_physical_device.getFeatures();
    Rhi::DeviceFeatureMask device_features;
    device_features.SetBit(Rhi::DeviceFeature::PresentToWindow,      IsExtensionSupported(VK_KHR_SWAPCHAIN_EXTENSION_NAME));
    device_features.SetBit(Rhi::DeviceFeature::AnisotropicFiltering, vk_device_features.samplerAnisotropy);
    device_features.SetBit(Rhi::DeviceFeature::ImageCubeArray,       vk_device_features.imageCubeArray);
    return device_features;
}

} // namespace Methane::Graphics::Vulkan
