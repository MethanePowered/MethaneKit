/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/DescriptorManagerVK.h
Vulkan descriptor manager with descriptor sets allocator.

******************************************************************************/

#pragma once

#include <Methane/Graphics/DescriptorManagerBase.h>

#include <Tracy.hpp>
#include <magic_enum.hpp>
#include <vulkan/vulkan.hpp>

#include <map>
#include <optional>
#include <mutex>

// Uncomment to enable deferred program bindings initialization
#define DEFERRED_PROGRAM_BINDINGS_INITIALIZATION

namespace Methane::Graphics
{

class ContextBase;
struct ProgramBindings;
struct IContextVK;

class DescriptorManagerVK final : public DescriptorManagerBase
{
public:
    using PoolSizeRatioByDescType = std::map<vk::DescriptorType, float>;

    DescriptorManagerVK(ContextBase& context, uint32_t pool_sets_count = 1000U,
                        const PoolSizeRatioByDescType& pool_size_ratio_by_desc_type = {
        { vk::DescriptorType::eSampler,              0.5f },
        { vk::DescriptorType::eCombinedImageSampler, 4.f  },
        { vk::DescriptorType::eSampledImage,         4.f  },
        { vk::DescriptorType::eStorageImage,         1.f  },
        { vk::DescriptorType::eUniformTexelBuffer,   1.f  },
        { vk::DescriptorType::eStorageTexelBuffer,   1.f  },
        { vk::DescriptorType::eUniformBuffer,        2.f  },
        { vk::DescriptorType::eStorageBuffer,        2.f  },
        { vk::DescriptorType::eUniformBufferDynamic, 1.f  },
        { vk::DescriptorType::eStorageBufferDynamic, 1.f  },
        { vk::DescriptorType::eInputAttachment,      0.5f }
    });

    // DescriptorManager overrides
#ifndef DEFERRED_PROGRAM_BINDINGS_INITIALIZATION
    void CompleteInitialization() override { /* intentionally uninitialized */}
#endif
    void Release() override;

    void SetDescriptorPoolSizeRatio(vk::DescriptorType descriptor_type, float size_ratio);
    vk::DescriptorSet AllocDescriptorSet(vk::DescriptorSetLayout layout);

private:
    vk::DescriptorPool CreateDescriptorPool();
    vk::DescriptorPool AcquireDescriptorPool();
    const IContextVK& GetContextVK() const noexcept;

    uint32_t                              m_pool_sets_count;
    PoolSizeRatioByDescType               m_pool_size_ratio_by_desc_type;
    std::vector<vk::UniqueDescriptorPool> m_vk_descriptor_pools;
    std::vector<vk::DescriptorPool>       m_vk_used_pools;
    std::vector<vk::DescriptorPool>       m_vk_free_pools;
    vk::DescriptorPool                    m_vk_current_pool;
    TracyLockable(std::mutex,             m_descriptor_pool_mutex)
};

} // namespace Methane::Graphics
