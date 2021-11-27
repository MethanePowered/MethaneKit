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

FILE: Methane/Graphics/ResourceManagerDX.h
Resource manager used as a central place for creating and accessing descriptor heaps
and deferred releasing of GPU resource.

******************************************************************************/

#pragma once

#include "DescriptorHeapDX.h"

#include <Methane/Graphics/ResourceManager.h>
#include <Methane/Graphics/ResourceBase.h>
#include <Methane/Graphics/ProgramBase.h>
#include <Methane/Instrumentation.h>

#include <magic_enum.hpp>
#include <array>
#include <mutex>

namespace Methane::Graphics
{

class ContextBase;

class ResourceManagerDX : public ResourceManager
{
public:
    using DescriptorHeapSizeByType = std::array<uint32_t, magic_enum::enum_count<DescriptorHeapDX::Type>() - 1>;

    struct Settings
    {
        bool                     deferred_heap_allocation = true;
        DescriptorHeapSizeByType default_heap_sizes;
        DescriptorHeapSizeByType shader_visible_heap_sizes;
    };

    explicit ResourceManagerDX(ContextBase& context);

    void Initialize(const Settings& settings);

    // ResourceManager overrides
    void CompleteInitialization() override;
    void Release() override;

    void SetDeferredHeapAllocation(bool deferred_heap_allocation);
    [[nodiscard]] bool IsDeferredHeapAllocation() const { return m_deferred_heap_allocation; }

    void AddProgramBindings(ProgramBindings& program_bindings);

    [[nodiscard]] uint32_t                     CreateDescriptorHeap(const DescriptorHeapDX::Settings& settings); // returns index of the created descriptor heap
    [[nodiscard]] DescriptorHeapDX&            GetDescriptorHeap(DescriptorHeapDX::Type type, Data::Index heap_index = 0);
    [[nodiscard]] DescriptorHeapDX&            GetDefaultShaderVisibleDescriptorHeap(DescriptorHeapDX::Type type) const;
    [[nodiscard]] DescriptorHeapSizeByType     GetDescriptorHeapSizes(bool get_allocated_size, bool for_shader_visible_heaps) const;

private:
    template<typename FuncType> // function void(DescriptorHeapDX& descriptor_heap)
    void ForEachDescriptorHeap(FuncType process_heap) const;

    using DescriptorHeapTypes = std::array<UniquePtrs<DescriptorHeapDX>, magic_enum::enum_count<DescriptorHeapDX::Type>() - 1>;

    bool                      m_deferred_heap_allocation = false;
    ContextBase&              m_context;
    DescriptorHeapTypes       m_descriptor_heap_types;
    WeakPtrs<ProgramBindings> m_program_bindings;
    TracyLockable(std::mutex, m_program_bindings_mutex)
};

} // namespace Methane::Graphics
