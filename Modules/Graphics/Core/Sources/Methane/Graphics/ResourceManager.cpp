/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/ResourceManager.cpp
Resource manager used as a central place for creating and accessing descriptor heaps
and deferred releasing of GPU resource.

******************************************************************************/

#include "ResourceManager.h"
#include "ContextBase.h"

#include <Methane/Data/Math.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <taskflow/taskflow.hpp>
#include <magic_enum.hpp>

namespace Methane::Graphics
{

inline void AddDescriptorHeap(Ptrs<DescriptorHeap>& desc_heaps, ContextBase& context, bool deferred_heap_allocation,
                              const ResourceManager::Settings& settings, DescriptorHeap::Type heap_type, bool is_shader_visible)
{
    const auto heap_type_idx = magic_enum::enum_integer(heap_type);
    const uint32_t heap_size = is_shader_visible ? settings.shader_visible_heap_sizes[heap_type_idx] : settings.default_heap_sizes[heap_type_idx];
    const DescriptorHeap::Settings heap_settings{ heap_type, heap_size, deferred_heap_allocation, is_shader_visible };
    desc_heaps.push_back(DescriptorHeap::Create(context, heap_settings));
}

ResourceManager::ResourceManager(ContextBase& context)
    : m_context(context)
{
    META_FUNCTION_TASK();
}

void ResourceManager::Initialize(const Settings& settings)
{
    META_FUNCTION_TASK();

    m_deferred_heap_allocation = settings.deferred_heap_allocation;
    for (const DescriptorHeap::Type heap_type : magic_enum::enum_values<DescriptorHeap::Type>())
    {
        if (heap_type == DescriptorHeap::Type::Undefined)
            continue;

        Ptrs<DescriptorHeap>& desc_heaps = m_descriptor_heap_types[magic_enum::enum_integer(heap_type)];
        desc_heaps.clear();

        // CPU only accessible descriptor heaps of all types are created for default resource creation
        AddDescriptorHeap(desc_heaps, m_context, m_deferred_heap_allocation, settings, heap_type, false);

        // GPU accessible descriptor heaps are created for program resource bindings
        if (DescriptorHeap::IsShaderVisibleHeapType(heap_type))
        {
            AddDescriptorHeap(desc_heaps, m_context, m_deferred_heap_allocation, settings, heap_type, true);
        }
    }
}

void ResourceManager::CompleteInitialization()
{
    META_FUNCTION_TASK();
    if (!IsDeferredHeapAllocation())
        return;

    std::lock_guard<LockableBase(std::mutex)> lock_guard(m_program_bindings_mutex);

    for (const Ptrs<DescriptorHeap>& desc_heaps : m_descriptor_heap_types)
    {
        for (const Ptr<DescriptorHeap>& desc_heap_ptr : desc_heaps)
        {
            META_CHECK_ARG_NOT_NULL(desc_heap_ptr);
            desc_heap_ptr->Allocate();
        }
    }

    const auto program_bindings_end_it = std::remove_if(m_program_bindings.begin(), m_program_bindings.end(),
        [](const WeakPtr<ProgramBindings>& program_bindings_wptr)
        { return program_bindings_wptr.expired(); }
    );

    m_program_bindings.erase(program_bindings_end_it, m_program_bindings.end());

    tf::Taskflow task_flow;
    task_flow.for_each_guided(m_program_bindings.begin(), m_program_bindings.end(),
        [](const WeakPtr<ProgramBindings>& program_bindings_wptr)
        {
            META_FUNCTION_TASK();
            Ptr<ProgramBindings> program_bindings_ptr = program_bindings_wptr.lock();
            META_CHECK_ARG_NOT_NULL(program_bindings_ptr);
            static_cast<ProgramBindingsBase&>(*program_bindings_ptr).CompleteInitialization();
        },
        Data::GetParallelChunkSizeAsInt(m_program_bindings.size(), 3)
    );
    m_context.GetParallelExecutor().run(task_flow).get();
}

void ResourceManager::Release()
{
    META_FUNCTION_TASK();
    for (Ptrs<DescriptorHeap>& desc_heaps : m_descriptor_heap_types)
    {
        desc_heaps.clear();
    }
}

void ResourceManager::SetDeferredHeapAllocation(bool deferred_heap_allocation)
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

void ResourceManager::AddProgramBindings(ProgramBindings& program_bindings)
{
    META_FUNCTION_TASK();

    std::lock_guard<LockableBase(std::mutex)> lock_guard(m_program_bindings_mutex);
#ifdef _DEBUG
    // This may cause performance drop on adding massive amount of program bindings,
    // so we assume that only different program bindings are added and check it in Debug builds only
    const auto program_bindings_it = std::find_if(m_program_bindings.begin(), m_program_bindings.end(),
        [&program_bindings](const WeakPtr<ProgramBindings>& program_bindings_ptr)
        { return !program_bindings_ptr.expired() && program_bindings_ptr.lock().get() == std::addressof(program_bindings); }
    );
    META_CHECK_ARG_DESCR("program_bindings", program_bindings_it == m_program_bindings.end(),
                         "program bindings instance was already added to resource manager");
#endif

    m_program_bindings.push_back(std::static_pointer_cast<ProgramBindingsBase>(static_cast<ProgramBindingsBase&>(program_bindings).GetBasePtr()));
}

uint32_t ResourceManager::CreateDescriptorHeap(const DescriptorHeap::Settings& settings)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_DESCR(settings.type, settings.type != DescriptorHeap::Type::Undefined,
                         "can not create 'Undefined' descriptor heap");

    Ptrs<DescriptorHeap>& desc_heaps = m_descriptor_heap_types[magic_enum::enum_integer(settings.type)];
    desc_heaps.push_back(DescriptorHeap::Create(m_context, settings));
    return static_cast<uint32_t>(desc_heaps.size() - 1);
}

const Ptr<DescriptorHeap>& ResourceManager::GetDescriptorHeapPtr(DescriptorHeap::Type type, Data::Index heap_index)
{
    META_FUNCTION_TASK();

    if (type == DescriptorHeap::Type::Undefined)
    {
        static const Ptr<DescriptorHeap> s_empty_ptr;
        return s_empty_ptr;
    }

    Ptrs<DescriptorHeap>& desc_heaps = m_descriptor_heap_types[magic_enum::enum_integer(type)];
    META_CHECK_ARG_LESS_DESCR(heap_index, desc_heaps.size(), "descriptor heap of type '{}' index is not valid", magic_enum::flags::enum_name(type));

    return desc_heaps[heap_index];
}

DescriptorHeap& ResourceManager::GetDescriptorHeap(DescriptorHeap::Type type, Data::Index heap_index)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_DESCR(type, type != DescriptorHeap::Type::Undefined,
                         "can not get reference to 'Undefined' descriptor heap");

    const Ptr<DescriptorHeap>& resource_heap_ptr = GetDescriptorHeapPtr(type, heap_index);
    META_CHECK_ARG_NOT_NULL_DESCR(resource_heap_ptr, "descriptor heap of type '{}' at index {} does not exist", magic_enum::flags::enum_name(type), heap_index);

    return *resource_heap_ptr;
}

const Ptr<DescriptorHeap>&  ResourceManager::GetDefaultShaderVisibleDescriptorHeapPtr(DescriptorHeap::Type type) const
{
    META_FUNCTION_TASK();

    if (type == DescriptorHeap::Type::Undefined)
    {
        static const Ptr<DescriptorHeap> s_empty_ptr;
        return s_empty_ptr;
    }

    const Ptrs<DescriptorHeap>& descriptor_heaps = m_descriptor_heap_types[magic_enum::enum_integer(type)];
    auto descriptor_heaps_it = std::find_if(descriptor_heaps.begin(), descriptor_heaps.end(),
        [](const Ptr<DescriptorHeap>& descriptor_heap_ptr)
        {
            META_CHECK_ARG_NOT_NULL(descriptor_heap_ptr);
            return descriptor_heap_ptr && descriptor_heap_ptr->GetSettings().shader_visible;
        });

    static const Ptr<DescriptorHeap> s_empty_heap_ptr;
    return descriptor_heaps_it != descriptor_heaps.end() ? *descriptor_heaps_it : s_empty_heap_ptr;
}

DescriptorHeap& ResourceManager::GetDefaultShaderVisibleDescriptorHeap(DescriptorHeap::Type type) const
{
    META_FUNCTION_TASK();

    const Ptr<DescriptorHeap>& resource_heap_ptr = GetDefaultShaderVisibleDescriptorHeapPtr(type);
    META_CHECK_ARG_NOT_NULL_DESCR(resource_heap_ptr, "There is no shader visible descriptor heap of type '{}'", magic_enum::flags::enum_name(type));

    return *resource_heap_ptr;
}

ResourceManager::DescriptorHeapSizeByType ResourceManager::GetDescriptorHeapSizes(bool get_allocated_size, bool for_shader_visible_heaps) const
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
void ResourceManager::ForEachDescriptorHeap(FuncType process_heap) const
{
    META_FUNCTION_TASK();
    for (const DescriptorHeap::Type desc_heaps_type : magic_enum::enum_values<DescriptorHeap::Type>())
    {
        if (desc_heaps_type == DescriptorHeap::Type::Undefined)
            continue;

        const Ptrs<DescriptorHeap>& desc_heaps = m_descriptor_heap_types[magic_enum::enum_integer(desc_heaps_type)];
        for (const Ptr<DescriptorHeap>& desc_heap_ptr : desc_heaps)
        {
            META_CHECK_ARG_NOT_NULL(desc_heap_ptr);
            const DescriptorHeap::Type heap_type = desc_heap_ptr->GetSettings().type;
            META_CHECK_ARG_EQUAL_DESCR(heap_type, desc_heaps_type,
                                       "wrong type of {} descriptor heap was found in container assuming heaps of {} type",
                                       magic_enum::flags::enum_name(heap_type),
                                       magic_enum::flags::enum_name(desc_heaps_type));
            process_heap(*desc_heap_ptr);
        }
        META_UNUSED(desc_heaps_type);
    }
}

} // namespace Methane::Graphics
