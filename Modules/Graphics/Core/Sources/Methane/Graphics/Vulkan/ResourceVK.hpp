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
#include "BlitCommandListVK.h"
#include "UtilsVK.hpp"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Graphics/ResourceBase.h>
#include <Methane/Graphics/CommandKit.h>
#include <Methane/Instrumentation.h>

#include <vulkan/vulkan.hpp>

namespace Methane::Graphics
{

template<typename ReourceBaseType, typename NativeResourceType, typename = std::enable_if_t<std::is_base_of_v<ResourceBase, ReourceBaseType>, void>>
class ResourceVK : public ReourceBaseType // NOSONAR - destructor in use
{
public:
    using UniqueResourceType = vk::UniqueHandle<NativeResourceType, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>;

    template<typename SettingsType>
    ResourceVK(const ContextBase& context, const SettingsType& settings,
               const ResourceBase::DescriptorByUsage&,
               UniqueResourceType&& vk_unique_resource)
        : ReourceBaseType(context, settings)
        , m_vk_device(GetContextVK().GetDeviceVK().GetNativeDevice())
        , m_vk_unique_resource(std::move(vk_unique_resource))
    {
        META_FUNCTION_TASK();
    }

    ~ResourceVK() override
    {
        META_FUNCTION_TASK();
        // Resource released callback has to be emitted before native resource is released
        Data::Emitter<IResourceCallback>::Emit(&IResourceCallback::OnResourceReleased, std::ref(*this));
    }

    ResourceVK(const ResourceVK&) = delete;
    ResourceVK(ResourceVK&&) = delete;

    bool operator=(const ResourceVK&) = delete;
    bool operator=(ResourceVK&&) = delete;

    // ObjectBase overide
    void SetName(const std::string& name) final
    {
        META_FUNCTION_TASK();
        if (ObjectBase::GetName() == name)
            return;

        ReourceBaseType::SetName(name);
        SetVulkanObjectName(m_vk_device, m_vk_unique_resource.get(), name.c_str());
    }

    // IResource overrides
    const Resource::DescriptorByUsage& GetDescriptorByUsage() const noexcept final
    {
        META_FUNCTION_TASK();
        static const Resource::DescriptorByUsage descriptor_by_usage;
        return descriptor_by_usage;
    }

    const Resource::Descriptor& GetDescriptor(Resource::Usage) const final
    {
        META_FUNCTION_NOT_IMPLEMENTED();
    }

    const vk::DeviceMemory& GetNativeDeviceMemory() const noexcept  { return m_vk_unique_device_memory.get(); }
    const NativeResourceType& GetNativeResource() const noexcept    { return m_vk_unique_resource.get(); }

protected:
    const IContextVK& GetContextVK() const noexcept                 { return dynamic_cast<const IContextVK&>(ResourceBase::GetContextBase()); }
    const vk::Device& GetNativeDevice() const noexcept              { return m_vk_device; }

    vk::UniqueDeviceMemory AllocateDeviceMemory(const vk::MemoryRequirements& memory_requirements, vk::MemoryPropertyFlags memory_property_flags)
    {
        META_FUNCTION_TASK();
        const Opt<uint32_t> memory_type_opt = GetContextVK().GetDeviceVK().FindMemoryType(memory_requirements.memoryTypeBits, memory_property_flags);
        if (!memory_type_opt)
            throw Resource::AllocationError(*this, "suitable memory type was not found");

        try
        {
            return GetNativeDevice().allocateMemoryUnique(vk::MemoryAllocateInfo(memory_requirements.size, *memory_type_opt));
        }
        catch(const vk::SystemError& error)
        {
            throw Resource::AllocationError(*this, error.what());
        }
    }

    void AllocateResourceMemory(const vk::MemoryRequirements& memory_requirements, vk::MemoryPropertyFlags memory_property_flags)
    {
        META_FUNCTION_TASK();
        m_vk_unique_device_memory.release();
        m_vk_unique_device_memory = AllocateDeviceMemory(memory_requirements, memory_property_flags);
    }

    void ResetNativeResource(UniqueResourceType&& vk_unique_resource)
    {
        META_FUNCTION_TASK();
        m_vk_unique_resource = std::move(vk_unique_resource);
    }

    BlitCommandListVK& PrepareResourceUpload(CommandQueue* sync_cmd_queue)
    {
        META_FUNCTION_TASK();
        META_UNUSED(sync_cmd_queue);
        auto& upload_cmd_list = dynamic_cast<BlitCommandListVK&>(ResourceBase::GetContext().GetUploadCommandKit().GetListForEncoding());
        upload_cmd_list.RetainResource(*this);
        return upload_cmd_list;
    }

private:
    vk::Device              m_vk_device;
    vk::UniqueDeviceMemory  m_vk_unique_device_memory;
    UniqueResourceType      m_vk_unique_resource;
};

} // namespace Methane::Graphics
