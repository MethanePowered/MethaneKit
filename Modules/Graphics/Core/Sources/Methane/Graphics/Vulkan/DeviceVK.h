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

FILE: Methane/Graphics/Vulkan/DeviceVK.h
Vulkan implementation of the device interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/DeviceBase.h>
#include <Methane/Graphics/CommandQueue.h>
#include <Methane/Data/RangeSet.hpp>

#include <vulkan/vulkan.hpp>

#include <map>

namespace Methane::Graphics
{

class DeviceVK final : public DeviceBase // NOSONAR
{
public:
    class IncompatibleException: public std::runtime_error
    {
    public:
        IncompatibleException(const std::string& incompatibility_reason)
            : std::runtime_error(incompatibility_reason)
        { }
    };

    class QueueFamilyReservation
    {
    public:
        QueueFamilyReservation(uint32_t family_index, vk::QueueFlagBits queue_flags, uint32_t queues_count, bool can_present_to_window = false);

        [[nodiscard]] uint32_t                  GetFamilyIndex() const noexcept      { return m_family_index; }
        [[nodiscard]] vk::QueueFlagBits         GetQueueFlags() const noexcept       { return m_queue_flags; }
        [[nodiscard]] uint32_t                  GetQueuesCount() const noexcept      { return m_queues_count; }
        [[nodiscard]] bool                      CanPresentToWindow() const noexcept  { return m_can_present_to_window; }
        [[nodiscard]] const std::vector<float>& GetPriorities() const noexcept       { return m_priorities; }
        [[nodiscard]] bool                      HasFreeQueueIndices() const noexcept { return !m_free_indices.IsEmpty(); }
        [[nodiscard]] uint32_t                  TakeFreeQueueIndex() const;
        [[nodiscard]] vk::DeviceQueueCreateInfo MakeDeviceQueueCreateInfo() const noexcept;

    private:
        uint32_t           m_family_index;
        vk::QueueFlagBits  m_queue_flags;
        uint32_t           m_queues_count;
        bool               m_can_present_to_window;
        std::vector<float> m_priorities;

        mutable Data::RangeSet<uint32_t> m_free_indices;
    };

    static Device::Features GetSupportedFeatures(const vk::PhysicalDevice& vk_physical_device);

    DeviceVK(const vk::PhysicalDevice& vk_physical_device, const vk::SurfaceKHR& vk_surface, const Capabilities& capabilities);
    ~DeviceVK() override;

    [[nodiscard]] const QueueFamilyReservation* GetQueueFamilyReservationPtr(CommandList::Type cmd_queue_type) const noexcept;
    [[nodiscard]] const QueueFamilyReservation& GetQueueFamilyReservation(CommandList::Type cmd_queue_type) const;

    const vk::PhysicalDevice& GetNativePhysicalDevice() const noexcept { return m_vk_physical_device; }
    const vk::Device&         GetNativeDevice() const noexcept         { return m_vk_device; }

private:
    using QueueFamilyReservationByType = std::map<CommandList::Type, QueueFamilyReservation>;

    void ReserveQueueFamily(CommandList::Type cmd_queue_type, uint32_t queues_count,
                            const std::vector<vk::QueueFamilyProperties>& vk_queue_family_properties,
                            const vk::SurfaceKHR& vk_surface = vk::SurfaceKHR());

    Capabilities                 m_device_caps;
    vk::PhysicalDevice           m_vk_physical_device;
    vk::Device                   m_vk_device;
    QueueFamilyReservationByType m_queue_family_reservation_by_type;
};

class SystemVK final : public SystemBase // NOSONAR
{
public:
    SystemVK();
    ~SystemVK() override;

    // System interface
    void CheckForChanges() override;
    const Ptrs<Device>& UpdateGpuDevices(const Platform::AppEnvironment& app_env, const Device::Capabilities& required_device_caps) override;
    const Ptrs<Device>& UpdateGpuDevices(const Device::Capabilities& required_device_caps) override;

    vk::DynamicLoader&       GetNativeLoader() noexcept       { return m_vk_loader; }
    const vk::DynamicLoader& GetNativeLoader() const noexcept { return m_vk_loader; }

    vk::Instance&       GetNativeInstance() noexcept          { return m_vk_instance; }
    const vk::Instance& GetNativeInstance() const noexcept    { return m_vk_instance; }

    const vk::SurfaceKHR& GetNativeSurface() const noexcept   { return m_vk_surface; }

private:
    vk::DynamicLoader m_vk_loader;
    vk::Instance      m_vk_instance;
    vk::SurfaceKHR    m_vk_surface;
};

} // namespace Methane::Graphics
