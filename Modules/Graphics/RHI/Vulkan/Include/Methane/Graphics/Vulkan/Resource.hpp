/******************************************************************************

Copyright 2019-2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/Resource.h
Vulkan implementation of the resource interface.

******************************************************************************/

#pragma once

#include "IResource.h"
#include "IContext.h"
#include "Device.h"
#include "TransferCommandList.h"
#include "Utils.hpp"

#include <Methane/Graphics/Base/Context.h>
#include <Methane/Graphics/Base/Resource.h>
#include <Methane/Graphics/RHI/ICommandKit.h>
#include <Methane/Data/EnumMaskUtil.hpp>
#include <Methane/Instrumentation.h>

#include <vulkan/vulkan.hpp>
#include <fmt/format.h>

#include <type_traits>
#include <cassert>

namespace Methane::Graphics::Vulkan
{

template<typename, typename = void>
constexpr bool is_defined_v = false;

template<typename T>
constexpr bool is_defined_v<T, std::void_t<decltype(sizeof(T))>> = true;

template<typename ResourceBaseType, typename NativeResourceType, bool is_unique_resource,
         typename = std::enable_if_t<std::is_base_of_v<Base::Resource, ResourceBaseType>, void>>
class Resource // NOSONAR - destructor in use
    : public ResourceBaseType
    , public IResource
{
    using ResourceStorageType = std::conditional_t<is_unique_resource,
                                                   vk::UniqueHandle<NativeResourceType, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>,
                                                   NativeResourceType>;

public:
    template<typename SettingsType, typename T = ResourceStorageType>
    Resource(const Base::Context& context, const SettingsType& settings, T&& vk_resource)
        : ResourceBaseType(context, settings, State::Undefined)
        , m_vk_device(GetVulkanContext().GetVulkanDevice().GetNativeDevice())
        , m_vk_resource(std::forward<T>(vk_resource))
    { }

    ~Resource() override
    {
        META_FUNCTION_TASK();
        m_upload_begin_transition_barriers_ptr.reset();
        m_upload_end_transition_barriers_ptr.reset();

        try
        {
            // Resource released callback has to be emitted before native resource is released
            Data::Emitter<Rhi::IResourceCallback>::Emit(&Rhi::IResourceCallback::OnResourceReleased, std::ref(*this));
        }
        catch (const std::exception& e)
        {
            META_UNUSED(e);
            META_LOG("WARNING: Unexpected error during resource destruction: {}", e.what());
            assert(false);
        }
    }

    Resource(const Resource&) = delete;
    Resource(Resource&&) = delete;

    bool operator=(const Resource&) = delete;
    bool operator=(Resource&&) = delete;

    // Base::Object override
    bool SetName(std::string_view name) override
    {
        META_FUNCTION_TASK();
        if (!ResourceBaseType::SetName(name))
            return false;

        if (const auto& vk_resource = GetNativeResource();
            vk_resource)
        {
            SetVulkanObjectName(m_vk_device, vk_resource, name);
        }

        for(const auto& [view_id, view_desc_ptr] : m_view_descriptor_by_view_id)
        {
            META_CHECK_ARG_NOT_NULL(view_desc_ptr);
            const std::string view_name = fmt::format("{} View for usage {}", name, Data::GetEnumMaskName(view_id.usage));

            if (const auto* image_view_desc_ptr = std::get_if<ResourceView::ImageViewDescriptor>(view_desc_ptr.get());
                image_view_desc_ptr)
            {
                SetVulkanObjectName(m_vk_device, image_view_desc_ptr->vk_view.get(), view_name.c_str());
                continue;
            }

            if (const auto* buffer_view_desc_ptr = std::get_if<ResourceView::BufferViewDescriptor>(view_desc_ptr.get());
                buffer_view_desc_ptr)
            {
                SetVulkanObjectName(m_vk_device, buffer_view_desc_ptr->vk_view.get(), view_name.c_str());
            }
        }

        return true;
    }

    const DescriptorByViewId& GetDescriptorByViewId() const noexcept final
    {
        static const DescriptorByViewId s_dummy_descriptor_by_view_id;
        return s_dummy_descriptor_by_view_id;
    }

    void RestoreDescriptorViews(const DescriptorByViewId&) final { /* intentionally uninitialized */ }

    // IResource overrides
    const IContext& GetVulkanContext() const noexcept final
    {
        return dynamic_cast<const IContext&>(Base::Resource::GetBaseContext());
    }

    const vk::DeviceMemory& GetNativeDeviceMemory() const noexcept final
    {
        return m_vk_unique_device_memory.get();
    }

    const vk::Device& GetNativeDevice() const noexcept final
    {
        return m_vk_device;
    }

    const Opt<uint32_t>& GetOwnerQueueFamilyIndex() const noexcept final
    {
        return m_owner_queue_family_index_opt;
    }

    const Ptr<ResourceView::ViewDescriptorVariant>& InitializeNativeViewDescriptor(const View::Id& view_id) final
    {
        META_FUNCTION_TASK();
        if (const auto it = m_view_descriptor_by_view_id.find(view_id);
            it != m_view_descriptor_by_view_id.end())
            return it->second;

        return m_view_descriptor_by_view_id.try_emplace(view_id, CreateNativeViewDescriptor(view_id)).first->second;
    }

    const auto& GetNativeResource() const noexcept
    {
        if constexpr (is_unique_resource)
            return m_vk_resource.get();
        else
            return m_vk_resource;
    }

protected:
    vk::UniqueDeviceMemory AllocateDeviceMemory(const vk::MemoryRequirements& memory_requirements, vk::MemoryPropertyFlags memory_property_flags)
    {
        META_FUNCTION_TASK();
        const Opt<uint32_t> memory_type_opt = GetVulkanContext().GetVulkanDevice().FindMemoryType(memory_requirements.memoryTypeBits, memory_property_flags);
        if (!memory_type_opt)
            throw IResource::AllocationError(*this, "suitable memory type was not found");

        try
        {
            return GetNativeDevice().allocateMemoryUnique(vk::MemoryAllocateInfo(memory_requirements.size, *memory_type_opt));
        }
        catch(const vk::SystemError& error)
        {
            throw IResource::AllocationError(*this, error.what());
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

    TransferCommandList& PrepareResourceTransfer(Rhi::ICommandQueue& target_cmd_queue, State transfer_state)
    {
        META_FUNCTION_TASK();
        const Rhi::ICommandKit& upload_cmd_kit = Base::Resource::GetContext().GetUploadCommandKit();
        auto& upload_cmd_list = dynamic_cast<TransferCommandList&>(upload_cmd_kit.GetListForEncoding());
        upload_cmd_list.RetainResource(*this);

        const bool owner_changed = SetOwnerQueueFamily(upload_cmd_kit.GetQueue().GetFamilyIndex(), m_upload_begin_transition_barriers_ptr);
        if (const bool state_changed = SetState(transfer_state, m_upload_begin_transition_barriers_ptr);
            (owner_changed || state_changed) &&
            m_upload_begin_transition_barriers_ptr && !m_upload_begin_transition_barriers_ptr->IsEmpty())
        {
            upload_cmd_list.SetResourceBarriers(*m_upload_begin_transition_barriers_ptr);
        }

        // If owner queue family has changed, resource barriers have to be also repeated on target command queue
        if (owner_changed && m_upload_begin_transition_barriers_ptr)
        {
            constexpr auto pre_upload_cmd_list_id = static_cast<Rhi::CommandListId>(Rhi::CommandListPurpose::PreUploadSync);
            Rhi::ICommandList& target_cmd_list = GetContext().GetDefaultCommandKit(target_cmd_queue).GetListForEncoding(pre_upload_cmd_list_id);
            target_cmd_list.SetResourceBarriers(*m_upload_begin_transition_barriers_ptr);
        }

        return upload_cmd_list;
    }

    void CompleteResourceTransfer(TransferCommandList& upload_cmd_list, State final_resource_state, Rhi::ICommandQueue& target_cmd_queue)
    {
        META_FUNCTION_TASK();
        const bool owner_changed = SetOwnerQueueFamily(target_cmd_queue.GetFamilyIndex(), m_upload_end_transition_barriers_ptr);
        const bool state_changed = SetState(final_resource_state, m_upload_end_transition_barriers_ptr);
        const bool upload_end_barriers_non_empty = m_upload_end_transition_barriers_ptr && !m_upload_end_transition_barriers_ptr->IsEmpty();
        if ((owner_changed || state_changed) && upload_end_barriers_non_empty)
        {
            upload_cmd_list.SetResourceBarriers(*m_upload_end_transition_barriers_ptr);
        }

        // If owner queue family has changed, resource barriers have to be also repeated on target command queue
        if (owner_changed && upload_end_barriers_non_empty)
        {
            constexpr auto post_upload_cmd_list_id = static_cast<Rhi::CommandListId>(Rhi::CommandListPurpose::PostUploadSync);
            Rhi::ICommandList& target_cmd_list = GetContext().GetDefaultCommandKit(target_cmd_queue).GetListForEncoding(post_upload_cmd_list_id);
            target_cmd_list.SetResourceBarriers(*m_upload_end_transition_barriers_ptr);
        }
    }

    void ResetNativeViewDescriptors() { m_view_descriptor_by_view_id.clear(); }

    virtual Ptr<ResourceView::ViewDescriptorVariant> CreateNativeViewDescriptor(const View::Id& view_id) = 0;

private:
    using ViewDescriptorByViewId = std::map<ResourceView::Id, Ptr<ResourceView::ViewDescriptorVariant>>;

    vk::Device                   m_vk_device;
    vk::UniqueDeviceMemory       m_vk_unique_device_memory;
    ResourceStorageType          m_vk_resource;
    ViewDescriptorByViewId       m_view_descriptor_by_view_id;
    Opt<uint32_t>                m_owner_queue_family_index_opt;
    Ptr<Rhi::IResourceBarriers>  m_upload_begin_transition_barriers_ptr;
    Ptr<Rhi::IResourceBarriers>  m_upload_end_transition_barriers_ptr;
};

} // namespace Methane::Graphics::Vulkan
