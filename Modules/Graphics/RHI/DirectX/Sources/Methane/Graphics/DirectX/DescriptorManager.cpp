/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/DescriptorManager.cpp
Descriptor manager is a central place for creating and accessing descriptor heaps.

******************************************************************************/

#include <Methane/Graphics/DirectX/DescriptorManager.h>

#include <Methane/Graphics/Base/Context.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <magic_enum/magic_enum.hpp>
#include <ranges>

namespace Methane::Graphics::DirectX
{

inline void AddDescriptorHeap(UniquePtrs<DescriptorHeap>& desc_heaps, const Base::Context& context, bool deferred_heap_allocation,
                              const DescriptorManager::Settings& settings, DescriptorHeap::Type heap_type, bool is_shader_visible)
{
    const auto heap_type_idx = magic_enum::enum_integer(heap_type);
    const uint32_t heap_size = is_shader_visible ? settings.shader_visible_heap_sizes[heap_type_idx] : settings.default_heap_sizes[heap_type_idx];
    const DescriptorHeap::Settings heap_settings{ heap_type, heap_size, deferred_heap_allocation, is_shader_visible };
    desc_heaps.push_back(std::make_unique<DescriptorHeap>(context, heap_settings));
}

DescriptorManager::DescriptorManager(Base::Context& context)
    : Base::DescriptorManager(context)
{ }

void DescriptorManager::Initialize(const Settings& settings)
{
    META_FUNCTION_TASK();

    m_deferred_heap_allocation = settings.deferred_heap_allocation;
    for (const DescriptorHeap::Type heap_type : magic_enum::enum_values<DescriptorHeap::Type>())
    {
        if (heap_type == DescriptorHeap::Type::Undefined)
            continue;

        UniquePtrs<DescriptorHeap>& desc_heaps = m_descriptor_heap_types[magic_enum::enum_integer(heap_type)];
        desc_heaps.clear();

        // CPU only accessible descriptor heaps of all types are created for default resource creation
        AddDescriptorHeap(desc_heaps, GetContext(), m_deferred_heap_allocation, settings, heap_type, false);

        // GPU accessible descriptor heaps are created for program resource bindings
        if (DescriptorHeap::IsShaderVisibleHeapType(heap_type))
        {
            AddDescriptorHeap(desc_heaps, GetContext(), m_deferred_heap_allocation, settings, heap_type, true);
        }
    }
}

void DescriptorManager::CompleteInitialization()
{
    META_FUNCTION_TASK();
    if (!m_deferred_heap_allocation)
        return;

    GetContext().WaitForGpu(Rhi::IContext::WaitFor::RenderComplete);

    for (const UniquePtrs<DescriptorHeap>& desc_heaps : m_descriptor_heap_types)
    {
        for (const UniquePtr<DescriptorHeap>& desc_heap_ptr : desc_heaps)
        {
            META_CHECK_NOT_NULL(desc_heap_ptr);
            desc_heap_ptr->Allocate();
        }
    }

    Base::DescriptorManager::CompleteInitialization();

    // Enable deferred heap allocation in case if more resources will be created in runtime
    m_deferred_heap_allocation = true;
}

void DescriptorManager::Release()
{
    META_FUNCTION_TASK();
    Base::DescriptorManager::Release();

    for (UniquePtrs<DescriptorHeap>& desc_heaps : m_descriptor_heap_types)
    {
        desc_heaps.clear();
    }
}

void DescriptorManager::SetDeferredHeapAllocation(bool deferred_heap_allocation)
{
    META_FUNCTION_TASK();
    if (m_deferred_heap_allocation == deferred_heap_allocation)
        return;

    m_deferred_heap_allocation = deferred_heap_allocation;
    ForEachDescriptorHeap([deferred_heap_allocation](DescriptorHeap& descriptor_heap)
    {
        descriptor_heap.SetDeferredAllocation(deferred_heap_allocation);
    });
}

uint32_t DescriptorManager::CreateDescriptorHeap(const DescriptorHeap::Settings& settings)
{
    META_FUNCTION_TASK();
    META_CHECK_DESCR(settings.type, settings.type != DescriptorHeap::Type::Undefined,
                     "can not create 'Undefined' descriptor heap");

    UniquePtrs<DescriptorHeap>& desc_heaps = m_descriptor_heap_types[magic_enum::enum_integer(settings.type)];
    desc_heaps.push_back(std::make_unique<DescriptorHeap>(GetContext(), settings));
    return static_cast<uint32_t>(desc_heaps.size() - 1);
}

DescriptorHeap& DescriptorManager::GetDescriptorHeap(DescriptorHeap::Type type, Data::Index heap_index)
{
    META_FUNCTION_TASK();
    META_CHECK_DESCR(type, type != DescriptorHeap::Type::Undefined,
                     "can not get reference to 'Undefined' descriptor heap");

    const UniquePtrs<DescriptorHeap>& desc_heaps = m_descriptor_heap_types[magic_enum::enum_integer(type)];
    META_CHECK_LESS_DESCR(heap_index, desc_heaps.size(),
                          "descriptor heap of type '{}' index is not valid", magic_enum::enum_name(type));

    const UniquePtr<DescriptorHeap>& resource_heap_ptr = desc_heaps[heap_index];
    META_CHECK_NOT_NULL_DESCR(resource_heap_ptr,
                              "descriptor heap of type '{}' at index {} does not exist", magic_enum::enum_name(type), heap_index);

    return *resource_heap_ptr;
}

DescriptorHeap& DescriptorManager::GetDefaultShaderVisibleDescriptorHeap(DescriptorHeap::Type type) const
{
    META_FUNCTION_TASK();
    META_CHECK_DESCR(type, type != DescriptorHeap::Type::Undefined,
                     "can not get reference to 'Undefined' descriptor heap");

    const UniquePtrs<DescriptorHeap>& descriptor_heaps = m_descriptor_heap_types[magic_enum::enum_integer(type)];
    auto descriptor_heaps_it = std::ranges::find_if(descriptor_heaps,
                                            [](const UniquePtr<DescriptorHeap>& descriptor_heap_ptr)
                                            {
                                                META_CHECK_NOT_NULL(descriptor_heap_ptr);
                                                return descriptor_heap_ptr && descriptor_heap_ptr->GetSettings().shader_visible;
                                            });
    META_CHECK_FALSE_DESCR(descriptor_heaps_it == descriptor_heaps.end(),
                           "Can not find shader visible {} descriptor heap", magic_enum::enum_name(type));

    const UniquePtr<DescriptorHeap>& descriptor_heap_ptr = *descriptor_heaps_it;
    META_CHECK_NOT_NULL_DESCR(descriptor_heap_ptr,
                              "There is no shader visible descriptor heap of type '{}'", magic_enum::enum_name(type));

    return *descriptor_heap_ptr;
}

DescriptorManager::DescriptorHeapSizeByType DescriptorManager::GetDescriptorHeapSizes(bool get_allocated_size, bool for_shader_visible_heaps) const
{
    META_FUNCTION_TASK();

    DescriptorHeapSizeByType max_descriptor_heap_sizes = {};
    ForEachDescriptorHeap([&max_descriptor_heap_sizes, for_shader_visible_heaps, get_allocated_size](const DescriptorHeap& descriptor_heap)
    {
        if ( (for_shader_visible_heaps && !descriptor_heap.IsShaderVisible()) ||
            (!for_shader_visible_heaps &&  descriptor_heap.IsShaderVisible()) )
            return;

        const uint32_t heap_size = get_allocated_size ? descriptor_heap.GetAllocatedSize() : descriptor_heap.GetDeferredSize();
        uint32_t& max_desc_heap_size = max_descriptor_heap_sizes[magic_enum::enum_integer(descriptor_heap.GetSettings().type)];
        max_desc_heap_size = std::max(max_desc_heap_size, heap_size);
    });

    return max_descriptor_heap_sizes;
}

template<typename FuncType>
void DescriptorManager::ForEachDescriptorHeap(FuncType process_heap) const
{
    META_FUNCTION_TASK();
    for (const DescriptorHeap::Type desc_heaps_type : magic_enum::enum_values<DescriptorHeap::Type>())
    {
        if (desc_heaps_type == DescriptorHeap::Type::Undefined)
            continue;

        const UniquePtrs<DescriptorHeap>& desc_heaps = m_descriptor_heap_types[magic_enum::enum_integer(desc_heaps_type)];
        for (const UniquePtr<DescriptorHeap>& desc_heap_ptr : desc_heaps)
        {
            META_CHECK_NOT_NULL(desc_heap_ptr);
            const DescriptorHeap::Type heap_type = desc_heap_ptr->GetSettings().type;
            META_CHECK_EQUAL_DESCR(heap_type, desc_heaps_type,
                                   "wrong type of {} descriptor heap was found in container assuming heaps of {} type",
                                   magic_enum::enum_name(heap_type),
                                   magic_enum::enum_name(desc_heaps_type));
            process_heap(*desc_heap_ptr);
        }
        META_UNUSED(desc_heaps_type);
    }
}

} // namespace Methane::Graphics::DirectX
