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

FILE: Methane/Graphics/Vulkan/DescriptorManagerVK.cpp
Vulkan descriptor manager with descriptor sets allocator.

******************************************************************************/

#include <Methane/Graphics/DescriptorManagerVK.h>
#include <Methane/Graphics/ContextVK.h>
#include <Methane/Graphics/DeviceVK.h>

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Graphics/ProgramBindingsBase.h>
#include <Methane/Graphics/ICommandKit.h>
#include <Methane/Graphics/ICommandList.h>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

DescriptorManagerVK::DescriptorManagerVK(ContextBase& context, uint32_t pool_sets_count, const PoolSizeRatioByDescType& pool_size_ratio_by_desc_type)
    : DescriptorManagerBase(context, false)
    , m_pool_sets_count(pool_sets_count)
    , m_pool_size_ratio_by_desc_type(pool_size_ratio_by_desc_type)
{
    META_FUNCTION_TASK();
}

void DescriptorManagerVK::Release()
{
    META_FUNCTION_TASK();
    DescriptorManagerBase::Release();

    std::scoped_lock lock_guard(m_descriptor_pool_mutex);
    const vk::Device& vk_device = GetContextVk().GetDeviceVK().GetNativeDevice();
    for(vk::DescriptorPool& vk_pool : m_vk_used_pools)
    {
        vk_device.resetDescriptorPool(vk_pool);
        m_vk_free_pools.emplace_back(vk_pool);
    }
    m_vk_used_pools.clear();
    m_vk_current_pool = nullptr;
}

void DescriptorManagerVK::SetDescriptorPoolSizeRatio(vk::DescriptorType descriptor_type, float size_ratio)
{
    META_FUNCTION_TASK();
    m_pool_size_ratio_by_desc_type[descriptor_type] = size_ratio;
}

vk::DescriptorSet DescriptorManagerVK::AllocDescriptorSet(vk::DescriptorSetLayout layout)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_descriptor_pool_mutex);
    if (!m_vk_current_pool)
        m_vk_current_pool = AcquireDescriptorPool();

    const vk::Device& vk_device = GetContextVk().GetDeviceVK().GetNativeDevice();

    try
    {
        const auto descriptor_sets = vk_device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo(m_vk_current_pool, 1, &layout));
        if (!descriptor_sets.empty())
            return descriptor_sets.back();
    }
    catch(const vk::OutOfPoolMemoryError&)
    {
        // Exception is handled below try-catch block
        META_LOG("Out of descriptor pool memory, reallocating.");
    }
    catch(const vk::FragmentedPoolError&)
    {
        // Exception is handled below try-catch block
        META_LOG("Fragmented descriptor pool, reallocating.");
    }

    // Reallocate descriptor set for the new pool
    m_vk_current_pool = AcquireDescriptorPool();
    const auto descriptor_sets = vk_device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo(m_vk_current_pool, 1, &layout));
    META_CHECK_ARG_NOT_EMPTY(descriptor_sets);
    return descriptor_sets.back();
}

vk::DescriptorPool DescriptorManagerVK::CreateDescriptorPool()
{
    META_FUNCTION_TASK();
    std::vector<vk::DescriptorPoolSize> pool_sizes;
    pool_sizes.reserve(m_pool_size_ratio_by_desc_type.size());
    for (const auto& [desc_type, size_ratio] : m_pool_size_ratio_by_desc_type)
    {
        pool_sizes.emplace_back(desc_type, static_cast<uint32_t>(static_cast<float>(m_pool_sets_count) * size_ratio));
    }
    const vk::Device& vk_device = GetContextVk().GetDeviceVK().GetNativeDevice();
    m_vk_descriptor_pools.emplace_back(vk_device.createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo({}, m_pool_sets_count, pool_sizes)));
    return m_vk_descriptor_pools.back().get();
}

vk::DescriptorPool DescriptorManagerVK::AcquireDescriptorPool()
{
    META_FUNCTION_TASK();
    if (m_vk_free_pools.empty())
    {
        vk::DescriptorPool new_pool = CreateDescriptorPool();
        m_vk_used_pools.emplace_back(new_pool);
        return new_pool;
    }

    vk::DescriptorPool free_pool = m_vk_free_pools.back();
    m_vk_free_pools.pop_back();
    return free_pool;
}

const IContextVK& DescriptorManagerVK::GetContextVk()
{
    META_FUNCTION_TASK();
    if (m_vk_context_ptr)
        return *m_vk_context_ptr;

    m_vk_context_ptr = dynamic_cast<const IContextVK*>(&GetContext());
    return *m_vk_context_ptr;
}

} // namespace Methane::Graphics
