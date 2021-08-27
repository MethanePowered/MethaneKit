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

FILE: Methane/Graphics/Vulkan/ResourceVK.h
Vulkan implementation of the resource interface.

******************************************************************************/

#pragma once

#include "ContextVK.h"
#include "DeviceVK.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Graphics/ResourceBase.h>
#include <Methane/Instrumentation.h>

#include <vulkan/vulkan.hpp>

namespace Methane::Graphics
{

class ResourceBarriersVK : public Resource::Barriers
{
public:
    explicit ResourceBarriersVK(const Set& barriers) : Resource::Barriers(barriers) {}
};

template<typename ReourceBaseType, typename = std::enable_if_t<std::is_base_of_v<ResourceBase, ReourceBaseType>, void>>
class ResourceVK : public ReourceBaseType
{
public:
    template<typename SettingsType>
    ResourceVK(const ContextBase& context, const SettingsType& settings, const ResourceBase::DescriptorByUsage& descriptor_by_usage)
        : ReourceBaseType(context, settings, descriptor_by_usage)
        , m_vk_device(GetContextVK().GetDeviceVK().GetNativeDevice())
    {
        META_FUNCTION_TASK();
    }

    ~ResourceVK() override
    {
        META_FUNCTION_TASK();
        // Resource released callback has to be emitted before native resource is released
        Data::Emitter<IResourceCallback>::Emit(&IResourceCallback::OnResourceReleased, std::ref(*this));
        // FreeDeviceMemory() should be called in derived class destructor after native resource is released
    }

    ResourceVK(const ResourceVK&) = delete;
    ResourceVK(ResourceVK&&) = delete;

    bool operator=(const ResourceVK&) = delete;
    bool operator=(ResourceVK&&) = delete;

    const vk::DeviceMemory& GetNativeDeviceMemory() const noexcept  { return m_vk_device_memory; }

protected:
    const IContextVK& GetContextVK() const noexcept                 { return dynamic_cast<const IContextVK&>(ResourceBase::GetContextBase()); }
    const vk::Device& GetNativeDevice() const noexcept              { return m_vk_device; }

    void AllocateDeviceMemory(const vk::MemoryRequirements& memory_requirements, vk::MemoryPropertyFlags memory_property_flags)
    {
        META_FUNCTION_TASK();
        FreeDeviceMemory();

        const Opt<uint32_t> memory_type_opt = GetContextVK().GetDeviceVK().FindMemoryType(memory_requirements.memoryTypeBits, memory_property_flags);
        if (!memory_type_opt)
            throw Resource::AllocationError(*this, "suitable memory type was not found");

        try
        {
            m_vk_device_memory = GetContextVK().GetDeviceVK().GetNativeDevice().allocateMemory(vk::MemoryAllocateInfo(memory_requirements.size, *memory_type_opt));
        }
        catch(const vk::SystemError& error)
        {
            throw Resource::AllocationError(*this, error.what());
        }
    }

    void FreeDeviceMemory()
    {
        META_FUNCTION_TASK();
        if (!m_vk_device_memory)
            return;

        m_vk_device.freeMemory(m_vk_device_memory);
        m_vk_device_memory = nullptr;
    }

private:
    vk::Device       m_vk_device;
    vk::DeviceMemory m_vk_device_memory;
};

} // namespace Methane::Graphics
