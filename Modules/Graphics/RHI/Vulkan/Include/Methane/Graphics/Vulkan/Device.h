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

FILE: Methane/Graphics/Vulkan/Device.h
Vulkan implementation of the device interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/Device.h>
#include <Methane/Graphics/RHI/ICommandQueue.h>
#include <Methane/Data/RangeSet.hpp>
#include <Methane/Memory.hpp>

#include <vulkan/vulkan.hpp>
#include <map>

namespace Methane::Graphics::Vulkan
{

class QueueFamilyReservation // NOSONAR
{
public:
    QueueFamilyReservation(uint32_t family_index, vk::QueueFlags queue_flags, uint32_t queues_count, bool can_present_to_window = false);
    ~QueueFamilyReservation();

    [[nodiscard]] vk::DeviceQueueCreateInfo MakeDeviceQueueCreateInfo() const noexcept;
    [[nodiscard]] uint32_t                  GetFamilyIndex() const noexcept      { return m_family_index; }
    [[nodiscard]] vk::QueueFlags            GetQueueFlags() const noexcept       { return m_queue_flags; }
    [[nodiscard]] uint32_t                  GetQueuesCount() const noexcept      { return m_queues_count; }
    [[nodiscard]] bool                      CanPresentToWindow() const noexcept  { return m_can_present_to_window; }
    [[nodiscard]] const std::vector<float>& GetPriorities() const noexcept       { return m_priorities; }
    [[nodiscard]] bool                      HasFreeQueues() const noexcept       { return !m_free_indices.IsEmpty(); }
    [[nodiscard]] uint32_t                  ClaimQueueIndex() const;

    void ReleaseQueueIndex(uint32_t queue_index) const;
    void IncrementQueuesCount(uint32_t extra_queues_count) noexcept;

private:
    uint32_t           m_family_index;
    vk::QueueFlags     m_queue_flags;
    uint32_t           m_queues_count;
    bool               m_can_present_to_window;
    std::vector<float> m_priorities;

    mutable Data::RangeSet<uint32_t> m_free_indices;
};

class Device final : public Base::Device
{
public:
    class IncompatibleException: public std::runtime_error
    {
    public:
        using runtime_error::runtime_error;
    };

    struct SwapChainSupport
    {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> present_modes;
    };

    static Rhi::DeviceFeatureMask GetSupportedFeatures(const vk::PhysicalDevice& vk_physical_device);

    Device(const vk::PhysicalDevice& vk_physical_device, const vk::SurfaceKHR& vk_surface, const Capabilities& capabilities);

    // IObject interface
    bool SetName(const std::string& name) override;

    [[nodiscard]] const QueueFamilyReservation* GetQueueFamilyReservationPtr(Rhi::CommandListType cmd_queue_type) const noexcept;
    [[nodiscard]] const QueueFamilyReservation& GetQueueFamilyReservation(Rhi::CommandListType cmd_queue_type) const;
    [[nodiscard]] SwapChainSupport GetSwapChainSupportForSurface(const vk::SurfaceKHR& vk_surface) const noexcept;
    [[nodiscard]] Opt<uint32_t> FindMemoryType(uint32_t type_filter, vk::MemoryPropertyFlags property_flags) const noexcept;

    const vk::PhysicalDevice&        GetNativePhysicalDevice() const noexcept { return m_vk_physical_device; }
    const vk::Device&                GetNativeDevice() const noexcept         { return m_vk_unique_device.get(); }
    const vk::QueueFamilyProperties& GetNativeQueueFamilyProperties(uint32_t queue_family_index) const;

private:
    using QueueFamilyReservationByType = std::map<Rhi::CommandListType, Ptr<QueueFamilyReservation>>;

    void ReserveQueueFamily(Rhi::CommandListType cmd_queue_type, uint32_t queues_count,
                            std::vector<uint32_t>& reserved_queues_count_per_family,
                            const vk::SurfaceKHR& vk_surface = vk::SurfaceKHR());

    bool IsExtensionSupported(const std::vector<std::string_view>& required_extensions) const;

    vk::PhysicalDevice                     m_vk_physical_device;
    std::vector<vk::QueueFamilyProperties> m_vk_queue_family_properties;
    vk::UniqueDevice                       m_vk_unique_device;
    QueueFamilyReservationByType           m_queue_family_reservation_by_type;
};

class System final : public Base::System // NOSONAR - destructor is required in this class
{
public:
    System();
    ~System() override;

    // ISystem interface
    void CheckForChanges() override;
    const Ptrs<Rhi::IDevice>& UpdateGpuDevices(const Methane::Platform::AppEnvironment& app_env, const Rhi::DeviceCaps& required_device_caps) override;
    const Ptrs<Rhi::IDevice>& UpdateGpuDevices(const Rhi::DeviceCaps& required_device_caps) override;

    vk::DynamicLoader&       GetNativeLoader() noexcept       { return m_vk_loader; }
    const vk::DynamicLoader& GetNativeLoader() const noexcept { return m_vk_loader; }

    vk::Instance&       GetNativeInstance() noexcept          { return m_vk_unique_instance.get(); }
    const vk::Instance& GetNativeInstance() const noexcept    { return m_vk_unique_instance.get(); }

private:    
    vk::DynamicLoader                m_vk_loader;
    vk::UniqueInstance               m_vk_unique_instance;
    vk::UniqueDebugUtilsMessengerEXT m_vk_unique_debug_utils_messanger;
    vk::UniqueSurfaceKHR             m_vk_unique_surface;
};

} // namespace Methane::Graphics::Vulkan
