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

FILE: Methane/Graphics/DirectX12/DescriptorHeapDX.h
Descriptor Heap is a platform abstraction of DirectX 12 descriptor heaps.

******************************************************************************/

#pragma once

#include "Methane/Graphics/ObjectBase.h"

#include <Methane/Data/RangeSet.hpp>
#include <Methane/Data/Provider.h>
#include <Methane/Data/Emitter.hpp>
#include <Methane/Memory.hpp>
#include <Methane/Instrumentation.h>

#include <set>
#include <mutex>
#include <functional>

#include <wrl.h>
#include <d3d12.h>

namespace Methane::Graphics
{

namespace wrl = Microsoft::WRL;

class ContextBase;
class ResourceBase;
class DescriptorHeapDX;
struct IContextDX;

struct IDescriptorHeapCallback
{
    virtual void OnDescriptorHeapAllocated(DescriptorHeapDX& descriptor_heap) = 0;

    virtual ~IDescriptorHeapCallback() = default;
};

class DescriptorHeapDX : public Data::Emitter<IDescriptorHeapCallback> // NOSONAR - this class requires destructor
{
public:
    enum class Type : uint32_t
    {
        // Shader visible heap types
        ShaderResources = 0U,
        Samplers,

        // Other heap types
        RenderTargets,
        DepthStencil,

        // Always keep at the end
        Undefined
    };

    struct Settings
    {
        Type       type;
        Data::Size size;
        bool       deferred_allocation;
        bool       shader_visible;
    };

    using Range    = Methane::Data::Range<Data::Index>;

    struct Reservation
    {
        static constexpr size_t ranges_count = 3;
        using Ranges = std::array<Range, ranges_count>;

        Ref<DescriptorHeapDX> heap;
        Ranges              ranges;

        explicit Reservation(const Ref<DescriptorHeapDX>& heap);
        Reservation(const Ref<DescriptorHeapDX>& heap, const Ranges& ranges);

        [[nodiscard]] const Range& GetRange(size_t range_index) const { return ranges.at(range_index); }
    };

    DescriptorHeapDX(const ContextBase& context, const Settings& settings);
    ~DescriptorHeapDX() override;

    [[nodiscard]] ID3D12DescriptorHeap*       GetNativeDescriptorHeap() noexcept           { return m_cp_descriptor_heap.Get();  }
    [[nodiscard]] D3D12_DESCRIPTOR_HEAP_TYPE  GetNativeDescriptorHeapType() const noexcept { return m_descriptor_heap_type; }
    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetNativeCpuDescriptorHandle(uint32_t descriptor_index) const;
    [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetNativeGpuDescriptorHandle(uint32_t descriptor_index) const;

    Data::Index AddResource(const ResourceBase& resource);
    Data::Index ReplaceResource(const ResourceBase& resource, Data::Index at_index);
    void        RemoveResource(Data::Index at_index);
    void        Allocate();

    Range ReserveRange(Data::Size length);
    void  ReleaseRange(const Range& range);
    void  SetDeferredAllocation(bool deferred_allocation);

    [[nodiscard]] const Settings&     GetSettings() const                          { return m_settings; }
    [[nodiscard]] Data::Size          GetDeferredSize() const                      { return m_deferred_size; }
    [[nodiscard]] Data::Size          GetAllocatedSize() const                     { return m_allocated_size; }
    [[nodiscard]] const ResourceBase* GetResource(uint32_t descriptor_index) const { return m_resources[descriptor_index]; }
    [[nodiscard]] bool                IsShaderVisible() const                      { return m_settings.shader_visible && IsShaderVisibleHeapType(m_settings.type); }
    [[nodiscard]] static bool         IsShaderVisibleHeapType(Type heap_type)      { return heap_type == Type::ShaderResources || heap_type == Type::Samplers; }

private:
    const IContextDX& GetContextDX() const noexcept;

    using ResourcePtrs = std::vector<const ResourceBase*>;
    using RangeSet     = Data::RangeSet<Data::Index>;

    const ContextBase&                m_context;
    Settings                          m_settings;
    Data::Size                        m_deferred_size;
    Data::Size                        m_allocated_size = 0;
    ResourcePtrs                      m_resources;
    RangeSet                          m_free_ranges;
    TracyLockable(std::mutex,         m_modification_mutex)
    D3D12_DESCRIPTOR_HEAP_TYPE        m_descriptor_heap_type;
    uint32_t                          m_descriptor_size;
    wrl::ComPtr<ID3D12DescriptorHeap> m_cp_descriptor_heap;
};

} // namespace Methane::Graphics
