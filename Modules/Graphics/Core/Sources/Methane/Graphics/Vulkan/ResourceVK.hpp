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

#include "ResourceVK.h"
#include "ContextVK.h"
#include "DeviceVK.h"
#include "BlitCommandListVK.h"
#include "UtilsVK.hpp"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Graphics/ResourceBase.h>
#include <Methane/Graphics/CommandKit.h>
#include <Methane/Instrumentation.h>

#include <vulkan/vulkan.hpp>

#include <type_traits>

namespace Methane::Graphics
{

template<typename, typename = void>
constexpr bool is_defined_v = false;

template<typename T>
constexpr bool is_defined_v<T, std::void_t<decltype(sizeof(T))>> = true;

template<typename ResourceBaseType, typename NativeResourceType, bool is_unique_resource, typename NativeViewType = void*,
         bool is_unique_view = std::is_class_v<NativeViewType>,
         typename = std::enable_if_t<std::is_base_of_v<ResourceBase, ResourceBaseType>, void>>
class ResourceVK // NOSONAR - destructor in use
    : public ResourceBaseType
    , public IResourceVK
{
    using ResourceStorageType = std::conditional_t<is_unique_resource,
                                                   vk::UniqueHandle<NativeResourceType, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>,
                                                   NativeResourceType>;

    using ViewStorageType = std::conditional_t<is_unique_view,
                                               vk::UniqueHandle<NativeViewType, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>,
                                               NativeViewType>;

public:
    template<typename SettingsType, typename T = ResourceStorageType>
    ResourceVK(const ContextBase& context, const SettingsType& settings, T&& vk_resource)
        : ResourceBaseType(context, settings)
        , m_vk_device(GetContextVK().GetDeviceVK().GetNativeDevice())
        , m_vk_resource(std::forward<T>(vk_resource))
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

        ResourceBaseType::SetName(name);

        if (m_vk_resource)
        {
            SetVulkanObjectName(m_vk_device, GetNativeResource(), name.c_str());
        }
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

    // IResourceVK overrides
    const vk::DeviceMemory& GetNativeDeviceMemory() const noexcept override
    {
        return m_vk_unique_device_memory.get();
    }

    const vk::Device& GetNativeDevice() const noexcept override
    {
        return m_vk_device;
    }

    const auto& GetNativeResource() const noexcept
    {
        if constexpr (is_unique_resource)
            return m_vk_resource.get();
        else
            return m_vk_resource;
    }

    template<typename T = NativeViewType>
    std::enable_if_t<std::is_class_v<T>, const T&> GetNativeView() const noexcept
    {
        if constexpr (is_unique_view)
            return m_vk_view.get();
        else
            return m_vk_view;
    }

protected:
    const IContextVK& GetContextVK() const noexcept
    {
        return dynamic_cast<const IContextVK&>(ResourceBase::GetContextBase());
    }

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

    template<typename T = ResourceStorageType>
    void ResetNativeResource(T&& vk_resource)
    {
        META_FUNCTION_TASK();
        m_vk_resource = std::forward<T>(vk_resource);
    }

    template<typename T = ViewStorageType, typename V = NativeViewType, typename = std::enable_if_t<std::is_class_v<V>>>
    void ResetNativeView(T&& vk_view)
    {
        META_FUNCTION_TASK();
        m_vk_view = std::forward<T>(vk_view);
    }

    BlitCommandListVK& PrepareResourceUpload(CommandQueue* sync_cmd_queue)
    {
        META_FUNCTION_TASK();
        auto& upload_cmd_list = dynamic_cast<BlitCommandListVK&>(ResourceBase::GetContext().GetUploadCommandKit().GetListForEncoding());
        upload_cmd_list.RetainResource(*this);

        if (SetState(State::Common, m_upload_sync_transition_barriers_ptr) && m_upload_sync_transition_barriers_ptr)
        {
            CommandList& sync_cmd_list = sync_cmd_queue
                                       ? GetContext().GetDefaultCommandKit(*sync_cmd_queue).GetListForEncoding()
                                       : upload_cmd_list;
            sync_cmd_list.SetResourceBarriers(*m_upload_sync_transition_barriers_ptr);
        }

        if (SetState(State::CopyDest, m_upload_begin_transition_barriers_ptr) && m_upload_begin_transition_barriers_ptr)
        {
            upload_cmd_list.SetResourceBarriers(*m_upload_begin_transition_barriers_ptr);
        }

        return upload_cmd_list;
    }

private:
    vk::Device              m_vk_device;
    vk::UniqueDeviceMemory  m_vk_unique_device_memory;
    ResourceStorageType     m_vk_resource;
    ViewStorageType         m_vk_view;
    Ptr<Resource::Barriers> m_upload_sync_transition_barriers_ptr;
    Ptr<Resource::Barriers> m_upload_begin_transition_barriers_ptr;
};

} // namespace Methane::Graphics
