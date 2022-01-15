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

#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

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
        AddResourceBarrier(barrier.GetId(), barrier.GetStateChange());
    }
}

ResourceBarriers::AddResult ResourceBarriersVK::AddStateChange(const ResourceBarrier::Id& id, const ResourceBarrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    const auto      lock_guard = ResourceBarriers::Lock();
    const AddResult result     = ResourceBarriers::AddStateChange(id, state_change);

    switch (result)
    {
    case AddResult::Added:   AddResourceBarrier(id, state_change); break;
    case AddResult::Updated: UpdateResourceBarrier(id, state_change); break;
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
    const Resource::Type resource_type = resource.GetResourceType();
    switch (resource_type)
    {
    case Resource::Type::Buffer:  RemoveBufferMemoryBarrier(dynamic_cast<BufferVK&>(resource).GetNativeResource()); break;
    case Resource::Type::Texture: RemoveImageMemoryBarrier(dynamic_cast<ITextureVK&>(resource).GetNativeImage()); break;
    default: META_UNEXPECTED_ARG_DESCR(resource_type, "resource type is not supported by transitions");
    }

    id.GetResource().Disconnect(*this);
    return true;
}

void ResourceBarriersVK::OnResourceReleased(Resource& resource)
{
    META_FUNCTION_TASK();
    RemoveTransition(resource);
}

void ResourceBarriersVK::AddResourceBarrier(const ResourceBarrier::Id& id, const ResourceBarrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    Resource& resource = id.GetResource();
    resource.Connect(*this);

    const Resource::Type resource_type = resource.GetResourceType();
    switch (resource_type)
    {
    case Resource::Type::Buffer:  AddBufferMemoryBarrier(dynamic_cast<BufferVK&>(resource).GetNativeResource(), state_change); break;
    case Resource::Type::Texture: AddImageMemoryBarrier(dynamic_cast<ITextureVK&>(resource).GetNativeImage(),
                                                        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor), // TODO: get real data from texture
                                                        state_change); break;
    default: META_UNEXPECTED_ARG_DESCR(resource_type, "resource type is not supported by transitions");
    }
}

void ResourceBarriersVK::AddBufferMemoryBarrier(const vk::Buffer& vk_buffer, const ResourceBarrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    m_vk_buffer_memory_barriers.emplace_back(
        ConvertResourceStateToVulkanAccessFlags(state_change.GetStateBefore()),
        ConvertResourceStateToVulkanAccessFlags(state_change.GetStateAfter()),
        0U,
        0U,
        vk_buffer
    );
}

void ResourceBarriersVK::AddImageMemoryBarrier(const vk::Image& vk_image, const vk::ImageSubresourceRange& vk_sub_resource_range,
                                               const ResourceBarrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    m_vk_image_memory_barriers.emplace_back(
        ConvertResourceStateToVulkanAccessFlags(state_change.GetStateBefore()),
        ConvertResourceStateToVulkanAccessFlags(state_change.GetStateAfter()),
        ConvertResourceStateToVulkanImageLayout(state_change.GetStateBefore()),
        ConvertResourceStateToVulkanImageLayout(state_change.GetStateAfter()),
        0U,
        0U,
        vk_image,
        vk_sub_resource_range
    );
}

void ResourceBarriersVK::UpdateResourceBarrier(const ResourceBarrier::Id& id, const ResourceBarrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    Resource& resource = id.GetResource();
    const Resource::Type resource_type = resource.GetResourceType();
    switch (resource_type)
    {
    case Resource::Type::Buffer:  UpdateBufferMemoryBarrier(dynamic_cast<BufferVK&>(resource).GetNativeResource(), state_change); break;
    case Resource::Type::Texture: UpdateImageMemoryBarrier(dynamic_cast<ITextureVK&>(resource).GetNativeImage(), state_change); break;
    default: META_UNEXPECTED_ARG_DESCR(resource_type, "resource type is not supported by transitions");
    }
}

void ResourceBarriersVK::UpdateBufferMemoryBarrier(const vk::Buffer& vk_buffer, const ResourceBarrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    const auto vk_buffer_memory_barrier_it = std::find_if(m_vk_buffer_memory_barriers.begin(), m_vk_buffer_memory_barriers.end(),
                                                          [&vk_buffer](const vk::BufferMemoryBarrier& vk_buffer_barrier)
                                                          { return vk_buffer_barrier.buffer == vk_buffer; });
    META_CHECK_ARG_TRUE(vk_buffer_memory_barrier_it != m_vk_buffer_memory_barriers.end());
    vk_buffer_memory_barrier_it->setSrcAccessMask(ConvertResourceStateToVulkanAccessFlags(state_change.GetStateBefore()));
    vk_buffer_memory_barrier_it->setDstAccessMask(ConvertResourceStateToVulkanAccessFlags(state_change.GetStateAfter()));
}

void ResourceBarriersVK::UpdateImageMemoryBarrier(const vk::Image& vk_image, const ResourceBarrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    const auto vk_image_memory_barrier_it = std::find_if(m_vk_image_memory_barriers.begin(), m_vk_image_memory_barriers.end(),
                                                         [&vk_image](const vk::ImageMemoryBarrier& vk_image_barrier)
                                                         { return vk_image_barrier.image == vk_image; });
    META_CHECK_ARG_TRUE(vk_image_memory_barrier_it != m_vk_image_memory_barriers.end());
    vk_image_memory_barrier_it->setSrcAccessMask(ConvertResourceStateToVulkanAccessFlags(state_change.GetStateBefore()));
    vk_image_memory_barrier_it->setDstAccessMask(ConvertResourceStateToVulkanAccessFlags(state_change.GetStateAfter()));
    vk_image_memory_barrier_it->setOldLayout(ConvertResourceStateToVulkanImageLayout(state_change.GetStateBefore()));
    vk_image_memory_barrier_it->setNewLayout(ConvertResourceStateToVulkanImageLayout(state_change.GetStateAfter()));
}

void ResourceBarriersVK::RemoveBufferMemoryBarrier(const vk::Buffer& vk_buffer)
{
    META_FUNCTION_TASK();
    const auto vk_buffer_memory_barrier_it = std::find_if(m_vk_buffer_memory_barriers.begin(), m_vk_buffer_memory_barriers.end(),
                                                          [&vk_buffer](const vk::BufferMemoryBarrier& vk_buffer_barrier)
                                                          { return vk_buffer_barrier.buffer == vk_buffer; });
    META_CHECK_ARG_TRUE(vk_buffer_memory_barrier_it != m_vk_buffer_memory_barriers.end());
    m_vk_buffer_memory_barriers.erase(vk_buffer_memory_barrier_it);
}

void ResourceBarriersVK::RemoveImageMemoryBarrier(const vk::Image& vk_image)
{
    META_FUNCTION_TASK();
    const auto vk_image_memory_barrier_it = std::find_if(m_vk_image_memory_barriers.begin(), m_vk_image_memory_barriers.end(),
                                                         [&vk_image](const vk::ImageMemoryBarrier& vk_image_barrier)
                                                         { return vk_image_barrier.image == vk_image; });
    META_CHECK_ARG_TRUE(vk_image_memory_barrier_it != m_vk_image_memory_barriers.end());
    m_vk_image_memory_barriers.erase(vk_image_memory_barrier_it);
}

} // namespace Methane::Graphics
