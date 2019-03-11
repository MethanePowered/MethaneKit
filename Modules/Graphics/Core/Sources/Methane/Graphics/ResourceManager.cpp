/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
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
#include "Instrumentation.h"

#include <cassert>

using namespace Methane::Graphics;

ResourceManager::ResourceManager(ContextBase& context)
    : m_context(context)
    , m_sp_release_pool(ResourceBase::ReleasePool::Create())
{
    ITT_FUNCTION_TASK();
}

void ResourceManager::Initialize(const Settings& settings)
{
    ITT_FUNCTION_TASK();

    m_deferred_heap_allocation = settings.deferred_heap_allocation;
    for (uint32_t heap_type_idx = 0; heap_type_idx < static_cast<uint32_t>(DescriptorHeap::Type::Count); ++heap_type_idx)
    {
        DescriptorHeaps& desc_heaps = m_descriptor_heap_types[heap_type_idx];
        desc_heaps.clear();

        // CPU only accessible descriptor heaps of all types are created for default resource creation
        const uint32_t                  heap_size = settings.default_heap_sizes[heap_type_idx];
        const DescriptorHeap::Type      heap_type = static_cast<DescriptorHeap::Type>(heap_type_idx);
        const DescriptorHeap::Settings  heap_settings = { heap_type, heap_size, m_deferred_heap_allocation, false };
        desc_heaps.push_back(DescriptorHeap::Create(m_context, heap_settings));

        // GPU accessible descriptor heaps are created for program resource bindings
        if (DescriptorHeap::IsShaderVisibileHeapType(heap_type))
        {
            const DescriptorHeap::Settings heap_settings = { heap_type, heap_size * settings.max_binding_states_count, m_deferred_heap_allocation, true };
            desc_heaps.push_back(DescriptorHeap::Create(m_context, heap_settings));
        }
    }
}

void ResourceManager::CompleteInitialization()
{
    ITT_FUNCTION_TASK();

    for (const DescriptorHeaps& desc_heaps : m_descriptor_heap_types)
    {
        for (const DescriptorHeap::Ptr& sp_desc_heap : desc_heaps)
        {
            assert(!!sp_desc_heap);
            sp_desc_heap->Allocate();
        }
    }

    for (const Program::ResourceBindings::WeakPtr& wp_resource_bindings : m_deferred_resource_bindings)
    {
        Program::ResourceBindings::Ptr sp_resource_bindings = wp_resource_bindings.lock();
        if (!sp_resource_bindings)
            continue;

        static_cast<ProgramBase::ResourceBindingsBase&>(*sp_resource_bindings).CompleteInitialization();
    }

    m_deferred_resource_bindings.clear();
}

void ResourceManager::DeferResourceBindingsInitialization(Program::ResourceBindings& resource_bindings)
{
    ITT_FUNCTION_TASK();
    m_deferred_resource_bindings.push_back(static_cast<ProgramBase::ResourceBindingsBase&>(resource_bindings).GetPtr());
}

uint32_t ResourceManager::CreateDescriptorHeap(const DescriptorHeap::Settings& settings)
{
    ITT_FUNCTION_TASK();

    if (settings.type == DescriptorHeap::Type::Undefined ||
        settings.type == DescriptorHeap::Type::Count)
    {
        throw std::invalid_argument("Can not create \"Undefined\" descriptor heap.");
    }
    DescriptorHeaps& desc_heaps = m_descriptor_heap_types[static_cast<size_t>(settings.type)];
    desc_heaps.push_back(DescriptorHeap::Create(m_context, settings));
    return static_cast<uint32_t>(desc_heaps.size() - 1);
}

const DescriptorHeap::Ptr& ResourceManager::GetDescriptorHeapPtr(DescriptorHeap::Type type, uint32_t heap_index)
{
    ITT_FUNCTION_TASK();

    if (type == DescriptorHeap::Type::Undefined ||
        type == DescriptorHeap::Type::Count)
    {
        static const DescriptorHeap::Ptr empty_ptr;
        return empty_ptr;
    }

    DescriptorHeaps& desc_heaps = m_descriptor_heap_types[static_cast<size_t>(type)];
    if (heap_index >= desc_heaps.size())
    {
        throw std::invalid_argument("There is no \"" + DescriptorHeap::GetTypeName(type) +
                                    "\" descriptor heap at index " + std::to_string(heap_index) +
                                    " (there are only " + std::to_string(desc_heaps.size()) + " heaps of this type).");
    }
    return desc_heaps[heap_index];
}

DescriptorHeap& ResourceManager::GetDescriptorHeap(DescriptorHeap::Type type, uint32_t heap_index)
{
    ITT_FUNCTION_TASK();

    if (type == DescriptorHeap::Type::Undefined ||
        type == DescriptorHeap::Type::Count)
    {
        throw std::invalid_argument("Can not get reference to \"Undefined\" descriptor heap.");
    }
    const DescriptorHeap::Ptr& sp_resource_heap = GetDescriptorHeapPtr(type, heap_index);
    if (!sp_resource_heap)
    {
        throw std::invalid_argument("Descriptor heap of type \"" + DescriptorHeap::GetTypeName(type) +
                                    "\" and index " + std::to_string(heap_index) + " does not exist.");
    }
    return *sp_resource_heap;
}

const DescriptorHeap::Ptr&  ResourceManager::GetDefaultShaderVisibleDescriptorHeapPtr(DescriptorHeap::Type type)
{
    ITT_FUNCTION_TASK();

    if (type == DescriptorHeap::Type::Undefined ||
        type == DescriptorHeap::Type::Count)
    {
        static const DescriptorHeap::Ptr empty_ptr;
        return empty_ptr;
    }

    const DescriptorHeaps& descriptor_heaps = m_descriptor_heap_types[static_cast<uint32_t>(type)];
    auto descriptor_heaps_it = std::find_if(descriptor_heaps.begin(), descriptor_heaps.end(),
        [](const DescriptorHeap::Ptr& sp_descriptor_heap)
        {
            assert(sp_descriptor_heap);
            return sp_descriptor_heap && sp_descriptor_heap->GetSettings().shader_visible;
        });

    static const DescriptorHeap::Ptr empty_heap_ptr;
    return descriptor_heaps_it != descriptor_heaps.end() ? *descriptor_heaps_it : empty_heap_ptr;
}

DescriptorHeap& ResourceManager::GetDefaultShaderVisibleDescriptorHeap(DescriptorHeap::Type type)
{
    ITT_FUNCTION_TASK();

    const DescriptorHeap::Ptr& sp_resource_heap = GetDefaultShaderVisibleDescriptorHeapPtr(type);
    if (!sp_resource_heap)
    {
        throw std::invalid_argument("There is no shader visible descriptor heap of type \"" + DescriptorHeap::GetTypeName(type) + "\".");
    }
    return *sp_resource_heap;
}

ResourceBase::ReleasePool& ResourceManager::GetReleasePool()
{
    ITT_FUNCTION_TASK();

    assert(!!m_sp_release_pool);
    return static_cast<ResourceBase::ReleasePool&>(*m_sp_release_pool);
}
