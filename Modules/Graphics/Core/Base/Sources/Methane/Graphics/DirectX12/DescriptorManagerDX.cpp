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

FILE: Methane/Graphics/DescriptorManagerDX.cpp
Descriptor manager is a central place for creating and accessing descriptor heaps.

******************************************************************************/

#include "DescriptorManagerDX.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>

namespace Methane::Graphics
{

inline void AddDescriptorHeap(UniquePtrs<DescriptorHeapDX>& desc_heaps, const ContextBase& context, bool deferred_heap_allocation,
                              const DescriptorManagerDX::Settings& settings, DescriptorHeapDX::Type heap_type, bool is_shader_visible)
{
    const auto heap_type_idx = magic_enum::enum_integer(heap_type);
    const uint32_t heap_size = is_shader_visible ? settings.shader_visible_heap_sizes[heap_type_idx] : settings.default_heap_sizes[heap_type_idx];
    const DescriptorHeapDX::Settings heap_settings{ heap_type, heap_size, deferred_heap_allocation, is_shader_visible };
    desc_heaps.push_back(std::make_unique<DescriptorHeapDX>(context, heap_settings));
}

DescriptorManagerDX::DescriptorManagerDX(ContextBase& context)
    : DescriptorManagerBase(context)
{
    META_FUNCTION_TASK();
}

void DescriptorManagerDX::Initialize(const Settings& settings)
{
    META_FUNCTION_TASK();

    m_deferred_heap_allocation = settings.deferred_heap_allocation;
    for (const DescriptorHeapDX::Type heap_type : magic_enum::enum_values<DescriptorHeapDX::Type>())
    {
        if (heap_type == DescriptorHeapDX::Type::Undefined)
            continue;

        UniquePtrs<DescriptorHeapDX>& desc_heaps = m_descriptor_heap_types[magic_enum::enum_integer(heap_type)];
        desc_heaps.clear();

        // CPU only accessible descriptor heaps of all types are created for default resource creation
        AddDescriptorHeap(desc_heaps, GetContext(), m_deferred_heap_allocation, settings, heap_type, false);

        // GPU accessible descriptor heaps are created for program resource bindings
        if (DescriptorHeapDX::IsShaderVisibleHeapType(heap_type))
        {
            AddDescriptorHeap(desc_heaps, GetContext(), m_deferred_heap_allocation, settings, heap_type, true);
        }
    }
}

void DescriptorManagerDX::CompleteInitialization()
{
    META_FUNCTION_TASK();
    if (!m_deferred_heap_allocation)
        return;

    GetContext().WaitForGpu(IContext::WaitFor::RenderComplete);

    for (const UniquePtrs<DescriptorHeapDX>& desc_heaps : m_descriptor_heap_types)
    {
        for (const UniquePtr<DescriptorHeapDX>& desc_heap_ptr : desc_heaps)
        {
            META_CHECK_ARG_NOT_NULL(desc_heap_ptr);
            desc_heap_ptr->Allocate();
        }
    }

    DescriptorManagerBase::CompleteInitialization();

    // Enable deferred heap allocation in case if more resources will be created in runtime
    m_deferred_heap_allocation = true;
}

void DescriptorManagerDX::Release()
{
    META_FUNCTION_TASK();
    DescriptorManagerBase::Release();

    for (UniquePtrs<DescriptorHeapDX>& desc_heaps : m_descriptor_heap_types)
    {
        desc_heaps.clear();
    }
}

void DescriptorManagerDX::SetDeferredHeapAllocation(bool deferred_heap_allocation)
{
    META_FUNCTION_TASK();
    if (m_deferred_heap_allocation == deferred_heap_allocation)
        return;

    m_deferred_heap_allocation = deferred_heap_allocation;
    ForEachDescriptorHeap([deferred_heap_allocation](DescriptorHeapDX& descriptor_heap)
    {
        descriptor_heap.SetDeferredAllocation(deferred_heap_allocation);
    });
}

uint32_t DescriptorManagerDX::CreateDescriptorHeap(const DescriptorHeapDX::Settings& settings)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_DESCR(settings.type, settings.type != DescriptorHeapDX::Type::Undefined,
                         "can not create 'Undefined' descriptor heap");

    UniquePtrs<DescriptorHeapDX>& desc_heaps = m_descriptor_heap_types[magic_enum::enum_integer(settings.type)];
    desc_heaps.push_back(std::make_unique<DescriptorHeapDX>(GetContext(), settings));
    return static_cast<uint32_t>(desc_heaps.size() - 1);
}

DescriptorHeapDX& DescriptorManagerDX::GetDescriptorHeap(DescriptorHeapDX::Type type, Data::Index heap_index)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_DESCR(type, type != DescriptorHeapDX::Type::Undefined,
                         "can not get reference to 'Undefined' descriptor heap");

    const UniquePtrs<DescriptorHeapDX>& desc_heaps = m_descriptor_heap_types[magic_enum::enum_integer(type)];
    META_CHECK_ARG_LESS_DESCR(heap_index, desc_heaps.size(), "descriptor heap of type '{}' index is not valid", magic_enum::enum_name(type));

    const UniquePtr<DescriptorHeapDX>& resource_heap_ptr = desc_heaps[heap_index];
    META_CHECK_ARG_NOT_NULL_DESCR(resource_heap_ptr, "descriptor heap of type '{}' at index {} does not exist", magic_enum::enum_name(type), heap_index);

    return *resource_heap_ptr;
}

DescriptorHeapDX& DescriptorManagerDX::GetDefaultShaderVisibleDescriptorHeap(DescriptorHeapDX::Type type) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_DESCR(type, type != DescriptorHeapDX::Type::Undefined,
                         "can not get reference to 'Undefined' descriptor heap");

    const UniquePtrs<DescriptorHeapDX>& descriptor_heaps = m_descriptor_heap_types[magic_enum::enum_integer(type)];
    auto descriptor_heaps_it = std::find_if(descriptor_heaps.begin(), descriptor_heaps.end(),
                                            [](const UniquePtr<DescriptorHeapDX>& descriptor_heap_ptr)
                                            {
                                                META_CHECK_ARG_NOT_NULL(descriptor_heap_ptr);
                                                return descriptor_heap_ptr && descriptor_heap_ptr->GetSettings().shader_visible;
                                            });

    const UniquePtr<DescriptorHeapDX>& descriptor_heap_ptr = *descriptor_heaps_it;
    META_CHECK_ARG_NOT_NULL_DESCR(descriptor_heap_ptr, "There is no shader visible descriptor heap of type '{}'",
                                  magic_enum::enum_name(type));

    return *descriptor_heap_ptr;
}

DescriptorManagerDX::DescriptorHeapSizeByType DescriptorManagerDX::GetDescriptorHeapSizes(bool get_allocated_size, bool for_shader_visible_heaps) const
{
    META_FUNCTION_TASK();

    DescriptorHeapSizeByType max_descriptor_heap_sizes = {};
    ForEachDescriptorHeap([&max_descriptor_heap_sizes, for_shader_visible_heaps, get_allocated_size](const DescriptorHeapDX& descriptor_heap)
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
void DescriptorManagerDX::ForEachDescriptorHeap(FuncType process_heap) const
{
    META_FUNCTION_TASK();
    for (const DescriptorHeapDX::Type desc_heaps_type : magic_enum::enum_values<DescriptorHeapDX::Type>())
    {
        if (desc_heaps_type == DescriptorHeapDX::Type::Undefined)
            continue;

        const UniquePtrs<DescriptorHeapDX>& desc_heaps = m_descriptor_heap_types[magic_enum::enum_integer(desc_heaps_type)];
        for (const UniquePtr<DescriptorHeapDX>& desc_heap_ptr : desc_heaps)
        {
            META_CHECK_ARG_NOT_NULL(desc_heap_ptr);
            const DescriptorHeapDX::Type heap_type = desc_heap_ptr->GetSettings().type;
            META_CHECK_ARG_EQUAL_DESCR(heap_type, desc_heaps_type,
                                       "wrong type of {} descriptor heap was found in container assuming heaps of {} type",
                                       magic_enum::enum_name(heap_type),
                                       magic_enum::enum_name(desc_heaps_type));
            process_heap(*desc_heap_ptr);
        }
        META_UNUSED(desc_heaps_type);
    }
}

} // namespace Methane::Graphics
