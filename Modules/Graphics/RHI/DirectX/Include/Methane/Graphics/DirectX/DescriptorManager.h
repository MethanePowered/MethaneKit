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

FILE: Methane/Graphics/DescriptorManager.h
Descriptor manager is a central place for creating and accessing descriptor heaps.

******************************************************************************/

#pragma once

#include "DescriptorHeap.h"

#include <Methane/Graphics/Base/DescriptorManager.h>
#include <Methane/Graphics/Base/Program.h>
#include <Methane/Instrumentation.h>

#include <magic_enum.hpp>
#include <array>
#include <mutex>

namespace Methane::Graphics::Base
{

class Context;

} // namespace Methane::Graphics::Base

namespace Methane::Graphics::DirectX
{

class DescriptorManager : public Base::DescriptorManager
{
public:
    using DescriptorHeapSizeByType = std::array<uint32_t, magic_enum::enum_count<DescriptorHeap::Type>() - 1>;

    struct Settings
    {
        bool                     deferred_heap_allocation = true;
        DescriptorHeapSizeByType default_heap_sizes;
        DescriptorHeapSizeByType shader_visible_heap_sizes;
    };

    explicit DescriptorManager(Base::Context& context);

    void Initialize(const Settings& settings);

    // IDescriptorManager overrides
    void CompleteInitialization() override;
    void Release() override;

    void SetDeferredHeapAllocation(bool deferred_heap_allocation);
    [[nodiscard]] bool IsDeferredHeapAllocation() const { return m_deferred_heap_allocation; }

    [[nodiscard]] uint32_t                 CreateDescriptorHeap(const DescriptorHeap::Settings& settings); // returns index of the created descriptor heap
    [[nodiscard]] DescriptorHeap&          GetDescriptorHeap(DescriptorHeap::Type type, Data::Index heap_index = 0);
    [[nodiscard]] DescriptorHeap&          GetDefaultShaderVisibleDescriptorHeap(DescriptorHeap::Type type) const;
    [[nodiscard]] DescriptorHeapSizeByType GetDescriptorHeapSizes(bool get_allocated_size, bool for_shader_visible_heaps) const;

private:
    template<typename FuncType> // function void(DescriptorHeap& descriptor_heap)
    void ForEachDescriptorHeap(FuncType process_heap) const;

    using DescriptorHeapTypes = std::array<UniquePtrs<DescriptorHeap>, magic_enum::enum_count<DescriptorHeap::Type>() - 1>;

    bool                      m_deferred_heap_allocation = false;
    DescriptorHeapTypes       m_descriptor_heap_types;
};

} // namespace Methane::Graphics::DirectX
