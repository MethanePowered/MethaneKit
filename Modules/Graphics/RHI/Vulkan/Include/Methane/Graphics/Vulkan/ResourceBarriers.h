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

#include <Methane/Graphics/Base/ResourceBarriers.h>
#include <Methane/Graphics/RHI/IResource.h>
#include <Methane/Data/Receiver.hpp>

#include <vulkan/vulkan.hpp>

namespace Methane::Graphics::Vulkan
{

class CommandQueue;
class Buffer;
class Texture;

class ResourceBarriers
    : public Base::ResourceBarriers
    , private Data::Receiver<Rhi::IResourceCallback>
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

    explicit ResourceBarriers(const Set& barriers);

    // IResourceBarriers overrides
    AddResult Add(const Barrier& barrier) override;
    bool Remove(const Barrier::Id& id) override;

    const NativePipelineBarrier& GetNativePipelineBarrierData(const CommandQueue& target_cmd_queue) const;

private:
    // IResourceCallback
    void OnResourceReleased(Rhi::IResource& resource) override;

    void SetResourceBarrier(const Barrier& barrier, bool is_new_barrier);
    void SetBufferMemoryBarrier(const Buffer& buffer, const Barrier& barrier);
    void SetImageMemoryBarrier(const Texture& texture, const Barrier& barrier);

    void AddBufferMemoryStateChangeBarrier(const Buffer& buffer, const Barrier::StateChange& state_change);
    void AddBufferMemoryOwnerChangeBarrier(const Buffer& buffer, const Barrier::OwnerChange& owner_change);
    void AddImageMemoryStateChangeBarrier(const Texture& texture, const Barrier::StateChange& state_change);
    void AddImageMemoryOwnerChangeBarrier(const Texture& texture, const Barrier::OwnerChange& owner_change);

    void RemoveBufferMemoryBarrier(const vk::Buffer& vk_buffer, Barrier::Type barrier_type);
    void RemoveImageMemoryBarrier(const vk::Image& vk_image, Barrier::Type barrier_type);

    void UpdateStageMasks();
    void UpdateStageMasks(const Barrier& barrier);

    NativePipelineBarrier m_vk_default_barrier;
    mutable std::map<uint32_t, NativePipelineBarrier> m_vk_barrier_by_queue_family;
};

} // namespace Methane::Graphics::Vulkan
