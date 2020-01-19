/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/ResourceManager.h
Resource manager used as a central place for creating and accessing descriptor heaps
and deferred releasing of GPU resource.

******************************************************************************/

#pragma once

#include "ResourceBase.h"
#include "DescriptorHeap.h"
#include "ProgramBase.h"

#include <array>
#include <mutex>

namespace Methane::Graphics
{

class ContextBase;

class ResourceManager
{
public:
    using DescriptorHeapSizeByType = std::array<uint32_t, static_cast<size_t>(DescriptorHeap::Type::Count)>;

    struct Settings
    {
        bool                     deferred_heap_allocation = true;
        DescriptorHeapSizeByType default_heap_sizes;
        DescriptorHeapSizeByType shader_visible_heap_sizes;
    };

    ResourceManager(ContextBase& context);
    ~ResourceManager() = default;

    void Initialize(const Settings& settings);
    void CompleteInitialization();
    void Release();

    bool DeferredHeapAllocationEnabled() const { return m_deferred_heap_allocation; }
    void DeferResourceBindingsInitialization(Program::ResourceBindings& resource_bindings);

    uint32_t                    CreateDescriptorHeap(const DescriptorHeap::Settings& settings); // returns index of the created descriptor heap
    const DescriptorHeap::Ptr&  GetDescriptorHeapPtr(DescriptorHeap::Type type, uint32_t heap_index = 0);
    DescriptorHeap&             GetDescriptorHeap(DescriptorHeap::Type type, uint32_t heap_index = 0);
    const DescriptorHeap::Ptr&  GetDefaultShaderVisibleDescriptorHeapPtr(DescriptorHeap::Type type);
    DescriptorHeap&             GetDefaultShaderVisibleDescriptorHeap(DescriptorHeap::Type type);
    DescriptorHeapSizeByType    GetDescriptorHeapSizes(bool get_allocated_size, bool for_shader_visible_heaps) const;
    ResourceBase::ReleasePool&  GetReleasePool();

protected:
    using DescriptorHeapTypes = std::array<Ptrs<DescriptorHeap>, static_cast<size_t>(DescriptorHeap::Type::Count)>;

    bool                                m_deferred_heap_allocation = false;
    ContextBase&                        m_context;
    DescriptorHeapTypes                 m_descriptor_heap_types;
    Ptr<ResourceBase::ReleasePool>      m_sp_release_pool;
    WeakPtrs<Program::ResourceBindings> m_deferred_resource_bindings;
    std::mutex                          m_deferred_resource_bindings_mutex;
};

} // namespace Methane::Graphics
