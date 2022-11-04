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

FILE: Methane/Graphics/Vulkan/ResourceVK.h
Vulkan implementation of the resource interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/ResourceBarriersBase.h>
#include <Methane/Graphics/IResource.h>
#include <Methane/Data/Receiver.hpp>

#include <vulkan/vulkan.hpp>

namespace Methane::Graphics
{

class CommandQueueVK;
class BufferVK;
struct ITextureVK;

class ResourceBarriersVK
    : public ResourceBarriersBase
    , private Data::Receiver<IResourceCallback>
{
public:
    struct NativePipelineBarrier
    {
        std::vector<vk::BufferMemoryBarrier> vk_buffer_memory_barriers;
        std::vector<vk::ImageMemoryBarrier>  vk_image_memory_barriers;
        std::vector<vk::MemoryBarrier>       vk_memory_barriers;
        vk::PipelineStageFlags               vk_src_stage_mask {};
        vk::PipelineStageFlags               vk_dst_stage_mask {};
    };

    explicit ResourceBarriersVK(const Set& barriers);

    // IResourceBarriers overrides
    AddResult Add(const ResourceBarrier::Id& id, const ResourceBarrier& barrier) override;
    bool Remove(const ResourceBarrier::Id& id) override;

    const NativePipelineBarrier& GetNativePipelineBarrierData(const CommandQueueVK& target_cmd_queue) const;

private:
    // IResourceCallback
    void OnResourceReleased(IResource& resource) override;

    void SetResourceBarrier(const ResourceBarrier::Id& id, const ResourceBarrier& barrier, bool is_new_barrier);
    void SetBufferMemoryBarrier(const BufferVK& buffer, const ResourceBarrier& barrier);
    void SetImageMemoryBarrier(const ITextureVK& texture, const ResourceBarrier& barrier);

    void AddBufferMemoryStateChangeBarrier(const BufferVK& buffer, const ResourceBarrier::StateChange& state_change);
    void AddBufferMemoryOwnerChangeBarrier(const BufferVK& buffer, const ResourceBarrier::OwnerChange& owner_change);
    void AddImageMemoryStateChangeBarrier(const ITextureVK& texture, const ResourceBarrier::StateChange& state_change);
    void AddImageMemoryOwnerChangeBarrier(const ITextureVK& texture, const ResourceBarrier::OwnerChange& owner_change);

    void RemoveBufferMemoryBarrier(const vk::Buffer& vk_buffer, ResourceBarrier::Type barrier_type);
    void RemoveImageMemoryBarrier(const vk::Image& vk_image, ResourceBarrier::Type barrier_type);

    void UpdateStageMasks();
    void UpdateStageMasks(const ResourceBarrier& barrier);

    NativePipelineBarrier m_vk_default_barrier;
    mutable std::map<uint32_t, NativePipelineBarrier> m_vk_barrier_by_queue_family;
};

} // namespace Methane::Graphics
