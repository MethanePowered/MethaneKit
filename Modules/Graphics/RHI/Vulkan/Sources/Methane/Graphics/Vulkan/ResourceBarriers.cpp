/******************************************************************************

Copyright 2020-2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/Resource.cpp
Vulkan implementation of the resource interface.

******************************************************************************/

#include <Methane/Graphics/Vulkan/ResourceBarriers.h>
#include <Methane/Graphics/Vulkan/Buffer.h>
#include <Methane/Graphics/Vulkan/Texture.h>
#include <Methane/Graphics/Vulkan/CommandQueue.h>

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

Ptr<IResourceBarriers> Rhi::IResourceBarriers::Create(const Set& barriers)
{
    META_FUNCTION_TASK();
    return std::make_shared<Vulkan::ResourceBarriers>(barriers);
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::Vulkan
{

static void UpdateBufferMemoryStateChangeBarrier(vk::BufferMemoryBarrier& vk_buffer_memory_barrier, const Rhi::ResourceBarrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    vk_buffer_memory_barrier.setSrcAccessMask(IResource::GetNativeAccessFlagsByResourceState(state_change.GetStateBefore()));
    vk_buffer_memory_barrier.setDstAccessMask(IResource::GetNativeAccessFlagsByResourceState(state_change.GetStateAfter()));
}

static void UpdateBufferMemoryOwnerChangeBarrier(vk::BufferMemoryBarrier& vk_buffer_memory_barrier, const Rhi::ResourceBarrier::OwnerChange& owner_change)
{
    META_FUNCTION_TASK();
    vk_buffer_memory_barrier.setSrcAccessMask(vk_buffer_memory_barrier.srcAccessMask | vk::AccessFlagBits::eMemoryWrite);
    vk_buffer_memory_barrier.setDstAccessMask(vk_buffer_memory_barrier.dstAccessMask | vk::AccessFlagBits::eMemoryRead);
    vk_buffer_memory_barrier.setSrcQueueFamilyIndex(owner_change.GetQueueFamilyBefore());
    vk_buffer_memory_barrier.setDstQueueFamilyIndex(owner_change.GetQueueFamilyAfter());
}

static void UpdateImageMemoryStateChangeBarrier(vk::ImageMemoryBarrier& vk_image_memory_barrier, const Rhi::ResourceBarrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    vk_image_memory_barrier.setSrcAccessMask(IResource::GetNativeAccessFlagsByResourceState(state_change.GetStateBefore()));
    vk_image_memory_barrier.setDstAccessMask(IResource::GetNativeAccessFlagsByResourceState(state_change.GetStateAfter()));
    vk_image_memory_barrier.setOldLayout(IResource::GetNativeImageLayoutByResourceState(state_change.GetStateBefore()));
    vk_image_memory_barrier.setNewLayout(IResource::GetNativeImageLayoutByResourceState(state_change.GetStateAfter()));
}

static void UpdateImageMemoryOwnerChangeBarrier(vk::ImageMemoryBarrier& vk_image_memory_barrier, const Rhi::ResourceBarrier::OwnerChange& owner_change)
{
    META_FUNCTION_TASK();
    vk_image_memory_barrier.setSrcAccessMask(vk_image_memory_barrier.srcAccessMask | vk::AccessFlagBits::eMemoryWrite);
    vk_image_memory_barrier.setDstAccessMask(vk_image_memory_barrier.dstAccessMask | vk::AccessFlagBits::eMemoryRead);
    vk_image_memory_barrier.setSrcQueueFamilyIndex(owner_change.GetQueueFamilyBefore());
    vk_image_memory_barrier.setDstQueueFamilyIndex(owner_change.GetQueueFamilyAfter());
}

ResourceBarriers::ResourceBarriers(const Set& barriers)
    : Base::ResourceBarriers(barriers)
{
    META_FUNCTION_TASK();
    for (const Rhi::ResourceBarrier barrier: barriers)
    {
        SetResourceBarrier(barrier.GetId(), barrier, true);
    }
}

Base::ResourceBarriers::AddResult ResourceBarriers::Add(const Rhi::ResourceBarrier::Id& id, const Rhi::ResourceBarrier& barrier)
{
    META_FUNCTION_TASK();
    const auto      lock_guard = Base::ResourceBarriers::Lock();
    const AddResult result     = Base::ResourceBarriers::Add(id, barrier);

    switch (result)
    {
    case AddResult::Added:   SetResourceBarrier(id, barrier, true); break;
    case AddResult::Updated: SetResourceBarrier(id, barrier, false); break;
    case AddResult::Existing: break;
    default: META_UNEXPECTED_ARG_RETURN(result, result);
    }

    return result;
}

bool ResourceBarriers::Remove(const Rhi::ResourceBarrier::Id& id)
{
    META_FUNCTION_TASK();
    const auto lock_guard = Base::ResourceBarriers::Lock();
    if (!Base::ResourceBarriers::Remove(id))
        return false;

    const Rhi::IResource& resource = id.GetResource();
    const Rhi::ResourceBarrier::Type   barrier_type  = id.GetType();

    switch (const Rhi::IResource::Type resource_type = resource.GetResourceType();
            resource_type)
    {
    case Rhi::ResourceType::Buffer:  RemoveBufferMemoryBarrier(dynamic_cast<const Buffer&>(resource).GetNativeResource(), barrier_type); break;
    case Rhi::ResourceType::Texture: RemoveImageMemoryBarrier(dynamic_cast<const Texture&>(resource).GetNativeImage(), barrier_type); break;
    default: META_UNEXPECTED_ARG_DESCR(resource_type, "resource type is not supported by transitions");
    }

    if (barrier_type == Rhi::ResourceBarrier::Type::StateTransition)
    {
        UpdateStageMasks();
        static_cast<Data::IEmitter<IResourceCallback>&>(id.GetResource()).Disconnect(*this);
    }

    m_vk_barrier_by_queue_family.clear();
    return true;
}

template<typename T>
void UpdateNativeBarrierAccessFlags(std::vector<T>& vk_native_barriers, vk::AccessFlags vk_supported_access_flags)
{
    for(T& vk_native_barrier : vk_native_barriers)
    {
        vk_native_barrier.srcAccessMask &= vk_supported_access_flags;
        vk_native_barrier.dstAccessMask &= vk_supported_access_flags;
    }
}

const ResourceBarriers::NativePipelineBarrier& ResourceBarriers::GetNativePipelineBarrierData(const CommandQueue& target_cmd_queue) const
{
    META_FUNCTION_TASK();
    const uint32_t cmd_queue_family_index = target_cmd_queue.GetFamilyIndex();

    const auto [barrier_it, is_added] = m_vk_barrier_by_queue_family.try_emplace(cmd_queue_family_index, m_vk_default_barrier);
    NativePipelineBarrier& native_pipeline_barrier = barrier_it->second;
    if (!is_added)
        return native_pipeline_barrier;

    const vk::PipelineStageFlags vk_supported_stage_flags  = target_cmd_queue.GetNativeSupportedStageFlags();
    native_pipeline_barrier.vk_src_stage_mask &= vk_supported_stage_flags;
    native_pipeline_barrier.vk_dst_stage_mask &= vk_supported_stage_flags;

    const vk::AccessFlags vk_supported_access_flags = target_cmd_queue.GetNativeSupportedAccessFlags();
    UpdateNativeBarrierAccessFlags(native_pipeline_barrier.vk_buffer_memory_barriers, vk_supported_access_flags);
    UpdateNativeBarrierAccessFlags(native_pipeline_barrier.vk_image_memory_barriers, vk_supported_access_flags);
    UpdateNativeBarrierAccessFlags(native_pipeline_barrier.vk_memory_barriers, vk_supported_access_flags);

    return native_pipeline_barrier;
}

void ResourceBarriers::OnResourceReleased(Rhi::IResource& resource)
{
    META_FUNCTION_TASK();
    RemoveStateTransition(resource);
}

void ResourceBarriers::SetResourceBarrier(const Rhi::ResourceBarrier::Id& id, const Rhi::ResourceBarrier& barrier, bool is_new_barrier)
{
    META_FUNCTION_TASK();
    const Rhi::IResource& resource = id.GetResource();
    switch (const Rhi::IResource::Type resource_type = resource.GetResourceType();
            resource_type)
    {
    case Rhi::IResource::Type::Buffer:  SetBufferMemoryBarrier(dynamic_cast<const Buffer&>(resource), barrier); break;
    case Rhi::IResource::Type::Texture: SetImageMemoryBarrier(dynamic_cast<const Texture&>(resource), barrier); break;
    default: META_UNEXPECTED_ARG_DESCR(resource_type, "resource type is not supported by transitions");
    }

    if (is_new_barrier)
    {
        static_cast<Data::IEmitter<IResourceCallback>&>(id.GetResource()).Connect(*this);
        UpdateStageMasks(barrier);
    }
    else
    {
        UpdateStageMasks();
    }

    m_vk_barrier_by_queue_family.clear();
}

void ResourceBarriers::SetBufferMemoryBarrier(const Buffer& buffer, const Rhi::ResourceBarrier& barrier)
{
    META_FUNCTION_TASK();
    const vk::Buffer& vk_buffer = buffer.GetNativeResource();
    const auto vk_buffer_memory_barrier_it = std::find_if(m_vk_default_barrier.vk_buffer_memory_barriers.begin(),
                                                          m_vk_default_barrier.vk_buffer_memory_barriers.end(),
                                                          [&vk_buffer](const vk::BufferMemoryBarrier& vk_buffer_barrier)
                                                          { return vk_buffer_barrier.buffer == vk_buffer; });

    if (vk_buffer_memory_barrier_it == m_vk_default_barrier.vk_buffer_memory_barriers.end())
    {
        switch(barrier.GetId().GetType())
        {
        case Rhi::ResourceBarrier::Type::StateTransition: AddBufferMemoryStateChangeBarrier(buffer, barrier.GetStateChange()); break;
        case Rhi::ResourceBarrier::Type::OwnerTransition: AddBufferMemoryOwnerChangeBarrier(buffer, barrier.GetOwnerChange()); break;
        }
    }
    else
    {
        switch (barrier.GetId().GetType())
        {
        case Rhi::ResourceBarrier::Type::StateTransition: UpdateBufferMemoryStateChangeBarrier(*vk_buffer_memory_barrier_it, barrier.GetStateChange()); break;
        case Rhi::ResourceBarrier::Type::OwnerTransition: UpdateBufferMemoryOwnerChangeBarrier(*vk_buffer_memory_barrier_it, barrier.GetOwnerChange()); break;
        }
    }
}

void ResourceBarriers::SetImageMemoryBarrier(const Texture& texture, const Rhi::ResourceBarrier& barrier)
{
    META_FUNCTION_TASK();
    const vk::Image& vk_image = texture.GetNativeImage();
    const auto vk_image_memory_barrier_it = std::find_if(m_vk_default_barrier.vk_image_memory_barriers.begin(),
                                                         m_vk_default_barrier.vk_image_memory_barriers.end(),
                                                         [&vk_image](const vk::ImageMemoryBarrier& vk_image_barrier)
                                                         { return vk_image_barrier.image == vk_image; });

    if (vk_image_memory_barrier_it == m_vk_default_barrier.vk_image_memory_barriers.end())
    {
        switch(barrier.GetId().GetType())
        {
        case Rhi::ResourceBarrier::Type::StateTransition: AddImageMemoryStateChangeBarrier(texture, barrier.GetStateChange()); break;
        case Rhi::ResourceBarrier::Type::OwnerTransition: AddImageMemoryOwnerChangeBarrier(texture, barrier.GetOwnerChange()); break;
        }
    }
    else
    {
        switch (barrier.GetId().GetType())
        {
        case Rhi::ResourceBarrier::Type::StateTransition: UpdateImageMemoryStateChangeBarrier(*vk_image_memory_barrier_it, barrier.GetStateChange()); break;
        case Rhi::ResourceBarrier::Type::OwnerTransition: UpdateImageMemoryOwnerChangeBarrier(*vk_image_memory_barrier_it, barrier.GetOwnerChange()); break;
        }
    }
}

void ResourceBarriers::AddBufferMemoryStateChangeBarrier(const Buffer& buffer, const Rhi::ResourceBarrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    m_vk_default_barrier.vk_buffer_memory_barriers.emplace_back(
        IResource::GetNativeAccessFlagsByResourceState(state_change.GetStateBefore()),
        IResource::GetNativeAccessFlagsByResourceState(state_change.GetStateAfter()),
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        buffer.GetNativeResource(),
        0U,
        buffer.GetSettings().size
    );
}

void ResourceBarriers::AddBufferMemoryOwnerChangeBarrier(const Buffer& buffer, const Rhi::ResourceBarrier::OwnerChange& owner_change)
{
    META_FUNCTION_TASK();
    const uint32_t family_index_before = owner_change.GetQueueFamilyBefore();
    const uint32_t family_index_after = owner_change.GetQueueFamilyAfter();
    if (family_index_before == family_index_after)
        return;

    m_vk_default_barrier.vk_buffer_memory_barriers.emplace_back(
        vk::AccessFlagBits::eMemoryWrite,
        vk::AccessFlagBits::eMemoryRead,
        family_index_before,
        family_index_after,
        buffer.GetNativeResource(),
        0U,
        buffer.GetSettings().size
    );
}

void ResourceBarriers::AddImageMemoryStateChangeBarrier(const Texture& texture, const Rhi::ResourceBarrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    m_vk_default_barrier.vk_image_memory_barriers.emplace_back(
        IResource::GetNativeAccessFlagsByResourceState(state_change.GetStateBefore()),
        IResource::GetNativeAccessFlagsByResourceState(state_change.GetStateAfter()),
        IResource::GetNativeImageLayoutByResourceState(state_change.GetStateBefore()),
        IResource::GetNativeImageLayoutByResourceState(state_change.GetStateAfter()),
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        texture.GetNativeImage(),
        texture.GetNativeSubresourceRange()
    );
}

void ResourceBarriers::AddImageMemoryOwnerChangeBarrier(const Texture& texture, const Rhi::ResourceBarrier::OwnerChange& owner_change)
{
    META_FUNCTION_TASK();
    const uint32_t family_index_before = owner_change.GetQueueFamilyBefore();
    const uint32_t family_index_after = owner_change.GetQueueFamilyAfter();
    if (family_index_before == family_index_after)
        return;

    const vk::ImageLayout image_layout = IResource::GetNativeImageLayoutByResourceState(texture.GetState());
    m_vk_default_barrier.vk_image_memory_barriers.emplace_back(
        vk::AccessFlagBits::eMemoryWrite,
        vk::AccessFlagBits::eMemoryRead,
        image_layout,
        image_layout,
        family_index_before,
        family_index_after,
        texture.GetNativeImage(),
        texture.GetNativeSubresourceRange()
    );
}

void ResourceBarriers::RemoveBufferMemoryBarrier(const vk::Buffer& vk_buffer, Rhi::ResourceBarrier::Type barrier_type)
{
    META_FUNCTION_TASK();
    const auto vk_buffer_memory_barrier_it = std::find_if(m_vk_default_barrier.vk_buffer_memory_barriers.begin(),
                                                          m_vk_default_barrier.vk_buffer_memory_barriers.end(),
                                                          [&vk_buffer](const vk::BufferMemoryBarrier& vk_buffer_barrier)
                                                          { return vk_buffer_barrier.buffer == vk_buffer; });
    if (vk_buffer_memory_barrier_it == m_vk_default_barrier.vk_buffer_memory_barriers.end())
        return;

    if (barrier_type == Rhi::ResourceBarrier::Type::OwnerTransition)
    {
        vk_buffer_memory_barrier_it->setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        vk_buffer_memory_barrier_it->setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    }
    else
    {
        m_vk_default_barrier.vk_buffer_memory_barriers.erase(vk_buffer_memory_barrier_it);
    }
}

void ResourceBarriers::RemoveImageMemoryBarrier(const vk::Image& vk_image, Rhi::ResourceBarrier::Type barrier_type)
{
    META_FUNCTION_TASK();
    const auto vk_image_memory_barrier_it = std::find_if(m_vk_default_barrier.vk_image_memory_barriers.begin(),
                                                         m_vk_default_barrier.vk_image_memory_barriers.end(),
                                                         [&vk_image](const vk::ImageMemoryBarrier& vk_image_barrier)
                                                         { return vk_image_barrier.image == vk_image; });
    if (vk_image_memory_barrier_it == m_vk_default_barrier.vk_image_memory_barriers.end())
        return;

    if (barrier_type == Rhi::ResourceBarrier::Type::OwnerTransition)
    {
        vk_image_memory_barrier_it->setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        vk_image_memory_barrier_it->setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    }
    else
    {
        m_vk_default_barrier.vk_image_memory_barriers.erase(vk_image_memory_barrier_it);
    }
}

void ResourceBarriers::UpdateStageMasks()
{
    META_FUNCTION_TASK();
    m_vk_default_barrier.vk_src_stage_mask = {};
    m_vk_default_barrier.vk_dst_stage_mask = {};
    for(const auto& [barrier_id, barrier] : Base::ResourceBarriers::GetMap())
    {
        UpdateStageMasks(barrier);
    }
}

void ResourceBarriers::UpdateStageMasks(const Rhi::ResourceBarrier& barrier)
{
    switch (barrier.GetId().GetType())
    {
    case Rhi::ResourceBarrier::Type::StateTransition:
        m_vk_default_barrier.vk_src_stage_mask |= IResource::GetNativePipelineStageFlagsByResourceState(barrier.GetStateChange().GetStateBefore());
        m_vk_default_barrier.vk_dst_stage_mask |= IResource::GetNativePipelineStageFlagsByResourceState(barrier.GetStateChange().GetStateAfter());
        break;

    case Rhi::ResourceBarrier::Type::OwnerTransition:
        m_vk_default_barrier.vk_src_stage_mask |= vk::PipelineStageFlagBits::eTransfer;
        break;
    }
}

} // namespace Methane::Graphics::Vulkan
