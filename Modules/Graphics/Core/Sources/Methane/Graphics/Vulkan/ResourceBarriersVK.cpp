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

FILE: Methane/Graphics/Vulkan/ResourceVK.cpp
Vulkan implementation of the resource interface.

******************************************************************************/

#include "ResourceBarriersVK.h"
#include "BufferVK.h"
#include "TextureVK.h"
#include "CommandQueueVK.h"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

[[nodiscard]]
static vk::AccessFlags ConvertResourceStateToVulkanAccessFlags(ResourceState resource_state)
{
    META_FUNCTION_TASK();
    switch (resource_state)
    {
    case ResourceState::Common:           return vk::AccessFlagBits::eNoneKHR;
    case ResourceState::VertexBuffer:     return vk::AccessFlagBits::eVertexAttributeRead;
    case ResourceState::ConstantBuffer:   return vk::AccessFlagBits::eUniformRead;
    case ResourceState::IndexBuffer:      return vk::AccessFlagBits::eIndexRead;
    case ResourceState::RenderTarget:     return vk::AccessFlagBits::eColorAttachmentRead |
                                                 vk::AccessFlagBits::eColorAttachmentWrite;
    case ResourceState::UnorderedAccess:  return vk::AccessFlagBits::eShaderRead |
                                                 vk::AccessFlagBits::eShaderWrite;
    case ResourceState::DepthWrite:       return vk::AccessFlagBits::eDepthStencilAttachmentWrite |
                                                 vk::AccessFlagBits::eDepthStencilAttachmentRead;
    case ResourceState::DepthRead:        return vk::AccessFlagBits::eDepthStencilAttachmentRead;
    case ResourceState::ShaderResource:   return vk::AccessFlagBits::eShaderRead;
    case ResourceState::IndirectArgument: return vk::AccessFlagBits::eIndirectCommandRead;
    case ResourceState::CopyDest:         return vk::AccessFlagBits::eTransferWrite;
    case ResourceState::CopySource:       return vk::AccessFlagBits::eTransferRead;
    case ResourceState::ResolveDest:      return vk::AccessFlagBits::eTransferWrite;
    case ResourceState::ResolveSource:    return vk::AccessFlagBits::eTransferRead;
    case ResourceState::Present:          return vk::AccessFlagBits::eNoneKHR;
    case ResourceState::GenericRead:      return vk::AccessFlagBits::eVertexAttributeRead |
                                                 vk::AccessFlagBits::eUniformRead |
                                                 vk::AccessFlagBits::eIndexRead |
                                                 vk::AccessFlagBits::eShaderRead |
                                                 vk::AccessFlagBits::eIndirectCommandRead |
                                                 vk::AccessFlagBits::eTransferRead;
    default: META_UNEXPECTED_ARG_DESCR_RETURN(resource_state, vk::AccessFlagBits::eNoneKHR, "unexpected resource state");
    }
}

[[nodiscard]]
static vk::ImageLayout ConvertResourceStateToVulkanImageLayout(ResourceState resource_state)
{
    META_FUNCTION_TASK();
    switch (resource_state)
    {
    case ResourceState::Common:          return vk::ImageLayout::eGeneral;
    case ResourceState::RenderTarget:    return vk::ImageLayout::eColorAttachmentOptimal;
    case ResourceState::InputAttachment: return vk::ImageLayout::eShaderReadOnlyOptimal;
    case ResourceState::UnorderedAccess: return vk::ImageLayout::eGeneral;
    case ResourceState::DepthWrite:      return vk::ImageLayout::eDepthStencilAttachmentOptimal;
    case ResourceState::DepthRead:       return vk::ImageLayout::eDepthStencilReadOnlyOptimal;
    case ResourceState::ShaderResource:  return vk::ImageLayout::eShaderReadOnlyOptimal;
    case ResourceState::CopyDest:        return vk::ImageLayout::eTransferDstOptimal;
    case ResourceState::CopySource:      return vk::ImageLayout::eTransferSrcOptimal;
    case ResourceState::ResolveDest:     return vk::ImageLayout::eTransferDstOptimal;
    case ResourceState::ResolveSource:   return vk::ImageLayout::eTransferSrcOptimal;
    case ResourceState::Present:         return vk::ImageLayout::ePresentSrcKHR;
    default: META_UNEXPECTED_ARG_DESCR_RETURN(resource_state, vk::ImageLayout::eUndefined, "unexpected resource state");
    }
}

[[nodiscard]]
static vk::PipelineStageFlags ConvertResourceStateToVulkanPipelineStageFlags(ResourceState resource_state)
{
    META_FUNCTION_TASK();
    switch (resource_state)
    {
    case ResourceState::Common:
        return vk::PipelineStageFlagBits::eAllCommands;
    case ResourceState::Present:
        return vk::PipelineStageFlagBits::eBottomOfPipe;
    case ResourceState::RenderTarget:
        return vk::PipelineStageFlagBits::eColorAttachmentOutput;
    case ResourceState::InputAttachment:
        return vk::PipelineStageFlagBits::eFragmentShader;
    case ResourceState::IndirectArgument:
        return vk::PipelineStageFlagBits::eDrawIndirect;
    case ResourceState::VertexBuffer:
    case ResourceState::IndexBuffer:
        return vk::PipelineStageFlagBits::eVertexInput;
    case ResourceState::ConstantBuffer:
    case ResourceState::UnorderedAccess:
    case ResourceState::ShaderResource:
        return vk::PipelineStageFlagBits::eVertexShader | // All possible shader stages
               vk::PipelineStageFlagBits::eFragmentShader;
    case ResourceState::CopyDest:
    case ResourceState::CopySource:
    case ResourceState::ResolveDest:
    case ResourceState::ResolveSource:
        return vk::PipelineStageFlagBits::eTransfer;
    case ResourceState::DepthWrite:
    case ResourceState::DepthRead:
        return vk::PipelineStageFlagBits::eEarlyFragmentTests |
               vk::PipelineStageFlagBits::eLateFragmentTests;
    case ResourceState::StreamOut:
        return {};
    default:
        META_UNEXPECTED_ARG_DESCR_RETURN(resource_state, vk::ImageLayout::eUndefined, "unexpected resource state");
    }
}

Ptr <ResourceBarriers> ResourceBarriers::Create(const Set& barriers)
{
    META_FUNCTION_TASK();
    return std::make_shared<ResourceBarriersVK>(barriers);
}

ResourceBarriersVK::ResourceBarriersVK(const Set& barriers)
    : ResourceBarriers(barriers)
{
    META_FUNCTION_TASK();
    for (const ResourceBarrier barrier: barriers)
    {
        SetResourceBarrier(barrier.GetId(), barrier, true);
    }
}

ResourceBarriers::AddResult ResourceBarriersVK::Add(const ResourceBarrier::Id& id, const ResourceBarrier& barrier)
{
    META_FUNCTION_TASK();
    const auto      lock_guard = ResourceBarriers::Lock();
    const AddResult result     = ResourceBarriers::Add(id, barrier);

    switch (result)
    {
    case AddResult::Added:   SetResourceBarrier(id, barrier, true); break;
    case AddResult::Updated: SetResourceBarrier(id, barrier, false); break;
    case AddResult::Existing: break;
    default: META_UNEXPECTED_ARG_RETURN(result, result);
    }

    return result;
}

bool ResourceBarriersVK::Remove(const ResourceBarrier::Id& id)
{
    META_FUNCTION_TASK();
    const auto lock_guard = ResourceBarriers::Lock();
    if (!ResourceBarriers::Remove(id))
        return false;

    Resource& resource = id.GetResource();
    const ResourceBarrier::Type barrier_type = id.GetType();
    const Resource::Type resource_type = resource.GetResourceType();
    switch (resource_type)
    {
    case Resource::Type::Buffer:  RemoveBufferMemoryBarrier(dynamic_cast<BufferVK&>(resource).GetNativeResource(), barrier_type); break;
    case Resource::Type::Texture: RemoveImageMemoryBarrier(dynamic_cast<ITextureVK&>(resource).GetNativeImage(), barrier_type); break;
    default: META_UNEXPECTED_ARG_DESCR(resource_type, "resource type is not supported by transitions");
    }

    if (barrier_type == ResourceBarrier::Type::StateTransition)
    {
        UpdateStageMasks();
        static_cast<Data::IEmitter<IResourceCallback>&>(id.GetResource()).Disconnect(*this);
    }
    return true;
}

void ResourceBarriersVK::OnResourceReleased(Resource& resource)
{
    META_FUNCTION_TASK();
    RemoveStateTransition(resource);
}

void ResourceBarriersVK::SetResourceBarrier(const ResourceBarrier::Id& id, const ResourceBarrier& barrier, bool is_new_barrier)
{
    META_FUNCTION_TASK();
    Resource& resource = id.GetResource();
    const Resource::Type resource_type = resource.GetResourceType();

    switch (resource_type)
    {
    case Resource::Type::Buffer:  SetBufferMemoryBarrier(dynamic_cast<BufferVK&>(resource), barrier); break;
    case Resource::Type::Texture: SetImageMemoryBarrier(dynamic_cast<ITextureVK&>(resource), barrier); break;
    default: META_UNEXPECTED_ARG_DESCR(resource_type, "resource type is not supported by transitions");
    }

    if (is_new_barrier)
    {
        static_cast<Data::IEmitter<IResourceCallback>&>(resource).Connect(*this);
        UpdateStageMasks(barrier);
    }
    else
    {
        UpdateStageMasks();
    }
}

void ResourceBarriersVK::SetBufferMemoryBarrier(BufferVK& buffer, const ResourceBarrier& barrier)
{
    META_FUNCTION_TASK();
    const vk::Buffer& vk_buffer = buffer.GetNativeResource();
    const auto vk_buffer_memory_barrier_it = std::find_if(m_vk_buffer_memory_barriers.begin(), m_vk_buffer_memory_barriers.end(),
                                                          [&vk_buffer](const vk::BufferMemoryBarrier& vk_buffer_barrier)
                                                          { return vk_buffer_barrier.buffer == vk_buffer; });

    if (vk_buffer_memory_barrier_it == m_vk_buffer_memory_barriers.end())
    {
        switch(barrier.GetId().GetType())
        {
        case ResourceBarrier::Type::StateTransition: AddBufferMemoryStateChangeBarrier(buffer, barrier.GetStateChange()); break;
        case ResourceBarrier::Type::OwnerTransition: AddBufferMemoryOwnerChangeBarrier(buffer, barrier.GetOwnerChange()); break;
        }
    }
    else
    {
        switch (barrier.GetId().GetType())
        {
        case ResourceBarrier::Type::StateTransition: UpdateBufferMemoryStateChangeBarrier(*vk_buffer_memory_barrier_it, barrier.GetStateChange()); break;
        case ResourceBarrier::Type::OwnerTransition: UpdateBufferMemoryOwnerChangeBarrier(*vk_buffer_memory_barrier_it, barrier.GetOwnerChange()); break;
        }
    }
}

void ResourceBarriersVK::SetImageMemoryBarrier(ITextureVK& texture, const ResourceBarrier& barrier)
{
    META_FUNCTION_TASK();
    const vk::Image& vk_image = texture.GetNativeImage();
    const auto vk_image_memory_barrier_it = std::find_if(m_vk_image_memory_barriers.begin(), m_vk_image_memory_barriers.end(),
                                                         [&vk_image](const vk::ImageMemoryBarrier& vk_image_barrier)
                                                             { return vk_image_barrier.image == vk_image; });

    if (vk_image_memory_barrier_it == m_vk_image_memory_barriers.end())
    {
        switch(barrier.GetId().GetType())
        {
        case ResourceBarrier::Type::StateTransition: AddImageMemoryStateChangeBarrier(texture, barrier.GetStateChange()); break;
        case ResourceBarrier::Type::OwnerTransition: AddImageMemoryOwnerChangeBarrier(texture, barrier.GetOwnerChange()); break;
        }
    }
    else
    {
        switch (barrier.GetId().GetType())
        {
        case ResourceBarrier::Type::StateTransition: UpdateImageMemoryStateChangeBarrier(*vk_image_memory_barrier_it, barrier.GetStateChange()); break;
        case ResourceBarrier::Type::OwnerTransition: UpdateImageMemoryOwnerChangeBarrier(*vk_image_memory_barrier_it, barrier.GetOwnerChange()); break;
        }
    }
}

void ResourceBarriersVK::AddBufferMemoryStateChangeBarrier(const BufferVK& buffer, const ResourceBarrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    m_vk_buffer_memory_barriers.emplace_back(
        ConvertResourceStateToVulkanAccessFlags(state_change.GetStateBefore()),
        ConvertResourceStateToVulkanAccessFlags(state_change.GetStateAfter()),
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        buffer.GetNativeResource(),
        0U,
        buffer.GetSettings().size
    );
}

void ResourceBarriersVK::AddBufferMemoryOwnerChangeBarrier(const BufferVK& buffer, const ResourceBarrier::OwnerChange& owner_change)
{
    META_FUNCTION_TASK();
    const uint32_t family_index_before = owner_change.GetQueueFamilyBefore();
    const uint32_t family_index_after = owner_change.GetQueueFamilyAfter();
    if (family_index_before == family_index_after)
        return;

    m_vk_buffer_memory_barriers.emplace_back(
        vk::AccessFlagBits::eMemoryWrite,
        vk::AccessFlagBits::eMemoryRead,
        family_index_before,
        family_index_after,
        buffer.GetNativeResource(),
        0U,
        buffer.GetSettings().size
    );
}

void ResourceBarriersVK::AddImageMemoryStateChangeBarrier(const ITextureVK& texture, const ResourceBarrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    m_vk_image_memory_barriers.emplace_back(
        ConvertResourceStateToVulkanAccessFlags(state_change.GetStateBefore()),
        ConvertResourceStateToVulkanAccessFlags(state_change.GetStateAfter()),
        ConvertResourceStateToVulkanImageLayout(state_change.GetStateBefore()),
        ConvertResourceStateToVulkanImageLayout(state_change.GetStateAfter()),
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        texture.GetNativeImage(),
        texture.GetNativeSubresourceRange()
    );
}

void ResourceBarriersVK::AddImageMemoryOwnerChangeBarrier(const ITextureVK& texture, const ResourceBarrier::OwnerChange& owner_change)
{
    META_FUNCTION_TASK();
    const uint32_t family_index_before = owner_change.GetQueueFamilyBefore();
    const uint32_t family_index_after = owner_change.GetQueueFamilyAfter();
    if (family_index_before == family_index_after)
        return;

    m_vk_image_memory_barriers.emplace_back(
        vk::AccessFlagBits::eMemoryWrite,
        vk::AccessFlagBits::eMemoryRead,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eUndefined,
        family_index_before,
        family_index_after,
        texture.GetNativeImage(),
        texture.GetNativeSubresourceRange()
    );
}

void ResourceBarriersVK::UpdateBufferMemoryStateChangeBarrier(vk::BufferMemoryBarrier& vk_buffer_memory_barrier, const ResourceBarrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    vk_buffer_memory_barrier.setSrcAccessMask(ConvertResourceStateToVulkanAccessFlags(state_change.GetStateBefore()));
    vk_buffer_memory_barrier.setDstAccessMask(ConvertResourceStateToVulkanAccessFlags(state_change.GetStateAfter()));
}

void ResourceBarriersVK::UpdateBufferMemoryOwnerChangeBarrier(vk::BufferMemoryBarrier& vk_buffer_memory_barrier, const ResourceBarrier::OwnerChange& owner_change)
{
    META_FUNCTION_TASK();
    vk_buffer_memory_barrier.setSrcAccessMask(vk_buffer_memory_barrier.srcAccessMask | vk::AccessFlagBits::eMemoryWrite);
    vk_buffer_memory_barrier.setDstAccessMask(vk_buffer_memory_barrier.dstAccessMask | vk::AccessFlagBits::eMemoryRead);
    vk_buffer_memory_barrier.setSrcQueueFamilyIndex(owner_change.GetQueueFamilyBefore());
    vk_buffer_memory_barrier.setDstQueueFamilyIndex(owner_change.GetQueueFamilyAfter());
}

void ResourceBarriersVK::UpdateImageMemoryStateChangeBarrier(vk::ImageMemoryBarrier& vk_image_memory_barrier, const ResourceBarrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    vk_image_memory_barrier.setSrcAccessMask(ConvertResourceStateToVulkanAccessFlags(state_change.GetStateBefore()));
    vk_image_memory_barrier.setDstAccessMask(ConvertResourceStateToVulkanAccessFlags(state_change.GetStateAfter()));
    vk_image_memory_barrier.setOldLayout(ConvertResourceStateToVulkanImageLayout(state_change.GetStateBefore()));
    vk_image_memory_barrier.setNewLayout(ConvertResourceStateToVulkanImageLayout(state_change.GetStateAfter()));
}

void ResourceBarriersVK::UpdateImageMemoryOwnerChangeBarrier(vk::ImageMemoryBarrier& vk_image_memory_barrier, const ResourceBarrier::OwnerChange& owner_change)
{
    META_FUNCTION_TASK();
    vk_image_memory_barrier.setSrcAccessMask(vk_image_memory_barrier.srcAccessMask | vk::AccessFlagBits::eMemoryWrite);
    vk_image_memory_barrier.setDstAccessMask(vk_image_memory_barrier.dstAccessMask | vk::AccessFlagBits::eMemoryRead);
    vk_image_memory_barrier.setSrcQueueFamilyIndex(owner_change.GetQueueFamilyBefore());
    vk_image_memory_barrier.setDstQueueFamilyIndex(owner_change.GetQueueFamilyAfter());
}

void ResourceBarriersVK::RemoveBufferMemoryBarrier(const vk::Buffer& vk_buffer, ResourceBarrier::Type barrier_type)
{
    META_FUNCTION_TASK();
    const auto vk_buffer_memory_barrier_it = std::find_if(m_vk_buffer_memory_barriers.begin(), m_vk_buffer_memory_barriers.end(),
                                                          [&vk_buffer](const vk::BufferMemoryBarrier& vk_buffer_barrier)
                                                          { return vk_buffer_barrier.buffer == vk_buffer; });
    if (vk_buffer_memory_barrier_it == m_vk_buffer_memory_barriers.end())
        return;

    if (barrier_type == ResourceBarrier::Type::OwnerTransition)
    {
        vk_buffer_memory_barrier_it->setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        vk_buffer_memory_barrier_it->setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    }
    else
    {
        m_vk_buffer_memory_barriers.erase(vk_buffer_memory_barrier_it);
    }
}

void ResourceBarriersVK::RemoveImageMemoryBarrier(const vk::Image& vk_image, ResourceBarrier::Type barrier_type)
{
    META_FUNCTION_TASK();
    const auto vk_image_memory_barrier_it = std::find_if(m_vk_image_memory_barriers.begin(), m_vk_image_memory_barriers.end(),
                                                         [&vk_image](const vk::ImageMemoryBarrier& vk_image_barrier)
                                                         { return vk_image_barrier.image == vk_image; });
    if (vk_image_memory_barrier_it == m_vk_image_memory_barriers.end())
        return;

    if (barrier_type == ResourceBarrier::Type::OwnerTransition)
    {
        vk_image_memory_barrier_it->setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        vk_image_memory_barrier_it->setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    }
    else
    {
        m_vk_image_memory_barriers.erase(vk_image_memory_barrier_it);
    }
}

void ResourceBarriersVK::UpdateStageMasks()
{
    META_FUNCTION_TASK();
    m_vk_src_stage_mask = {};
    m_vk_dst_stage_mask = {};
    for(const auto& [barrier_id, barrier] : ResourceBarriers::GetMap())
    {
        UpdateStageMasks(barrier);
    }
}

void ResourceBarriersVK::UpdateStageMasks(const ResourceBarrier& barrier)
{
    switch (barrier.GetId().GetType())
    {
    case ResourceBarrier::Type::StateTransition:
        m_vk_src_stage_mask |= ConvertResourceStateToVulkanPipelineStageFlags(barrier.GetStateChange().GetStateBefore());
        m_vk_dst_stage_mask |= ConvertResourceStateToVulkanPipelineStageFlags(barrier.GetStateChange().GetStateAfter());
        break;

    case ResourceBarrier::Type::OwnerTransition:
        m_vk_src_stage_mask |= vk::PipelineStageFlagBits::eTransfer;
        break;
    }
}

} // namespace Methane::Graphics
