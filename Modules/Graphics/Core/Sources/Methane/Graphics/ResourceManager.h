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

FILE: Methane/Graphics/ResourceManager.h
Resource manager used as a central place for creating and accessing descriptor heaps
and deferred releasing of GPU resource.

******************************************************************************/

#pragma once

#include "ResourceBase.h"
#include "DescriptorHeap.h"
#include "ProgramBase.h"

#include <Methane/Instrumentation.h>

#include <magic_enum.hpp>
#include <array>
#include <mutex>

namespace Methane::Graphics
{

class ContextBase;

class ResourceManager
{
public:
    using DescriptorHeapSizeByType = std::array<uint32_t, magic_enum::enum_count<DescriptorHeap::Type>() - 1>;

    struct Settings
    {
        bool                     deferred_heap_allocation = true;
        DescriptorHeapSizeByType default_heap_sizes;
        DescriptorHeapSizeByType shader_visible_heap_sizes;
    };

    explicit ResourceManager(ContextBase& context);
    ~ResourceManager() = default;

    void Initialize(const Settings& settings);
    void CompleteInitialization();
    void Release();

    void SetDeferredHeapAllocation(bool deferred_heap_allocation);
    bool IsDeferredHeapAllocation() const { return m_deferred_heap_allocation; }

    void AddProgramBindings(ProgramBindings& program_bindings);

    uint32_t                    CreateDescriptorHeap(const DescriptorHeap::Settings& settings); // returns index of the created descriptor heap
    const Ptr<DescriptorHeap>&  GetDescriptorHeapPtr(DescriptorHeap::Type type, Data::Index heap_index = 0);
    DescriptorHeap&             GetDescriptorHeap(DescriptorHeap::Type type, Data::Index heap_index = 0);
    const Ptr<DescriptorHeap>&  GetDefaultShaderVisibleDescriptorHeapPtr(DescriptorHeap::Type type) const;
    DescriptorHeap&             GetDefaultShaderVisibleDescriptorHeap(DescriptorHeap::Type type) const;
    DescriptorHeapSizeByType    GetDescriptorHeapSizes(bool get_allocated_size, bool for_shader_visible_heaps) const;

private:
    template<typename FuncType> // function void(DescriptorHeap& descriptor_heap)
    void ForEachDescriptorHeap(FuncType process_heap) const;

    using DescriptorHeapTypes = std::array<Ptrs<DescriptorHeap>, magic_enum::enum_count<DescriptorHeap::Type>() - 1>;

    bool                      m_deferred_heap_allocation = false;
    ContextBase&              m_context;
    DescriptorHeapTypes       m_descriptor_heap_types;
    TracyLockable(std::mutex, m_program_bindings_mutex);
    WeakPtrs<ProgramBindings> m_program_bindings;
};

} // namespace Methane::Graphics
