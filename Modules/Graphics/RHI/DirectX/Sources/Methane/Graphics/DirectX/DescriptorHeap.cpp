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

FILE: Methane/Graphics/DirectX/DescriptorHeap.cpp
Descriptor Heap is a platform abstraction of DirectX 12 descriptor heaps.

******************************************************************************/

#include <Methane/Graphics/DirectX/DescriptorHeap.h>
#include <Methane/Graphics/DirectX/Device.h>
#include <Methane/Graphics/DirectX/IContext.h>

#include <Methane/Graphics/Base/Resource.h>
#include <Methane/Graphics/Base/Context.h>
#include <Methane/Graphics/DirectX/ErrorHandling.h>
#include <Methane/Data/RangeUtils.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>
#include <directx/d3dx12_root_signature.h>
#include <cassert>

namespace Methane::Graphics::DirectX
{

static D3D12_DESCRIPTOR_HEAP_TYPE GetNativeHeapType(DescriptorHeapType type)
{
    META_FUNCTION_TASK();
    switch (type)
    {
    case DescriptorHeap::Type::ShaderResources: return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    case DescriptorHeap::Type::Samplers:        return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    case DescriptorHeap::Type::RenderTargets:   return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    case DescriptorHeap::Type::DepthStencil:    return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    default:                                      META_UNEXPECTED_ARG_RETURN(type, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES);
    }
}

DescriptorHeapReservation::DescriptorHeapReservation(const Ref<DescriptorHeap>& heap)
    : heap(heap)
{
    META_FUNCTION_TASK();
    std::fill(ranges.begin(), ranges.end(), DescriptorHeap::Range(0, 0));
}

DescriptorHeapReservation::DescriptorHeapReservation(const Ref<DescriptorHeap>& heap, const Ranges& ranges)
    : heap(heap)
    , ranges(ranges)
{ }

DescriptorHeap::DescriptorHeap(const Base::Context& context, const Settings& settings)
    : m_dx_context(dynamic_cast<const IContext&>(context))
    , m_settings(settings)
    , m_deferred_size(settings.size)
    , m_descriptor_heap_type(GetNativeHeapType(settings.type))
    , m_descriptor_size(m_dx_context.GetDirectDevice().GetNativeDevice()->GetDescriptorHandleIncrementSize(m_descriptor_heap_type))
{
    META_FUNCTION_TASK();
    if (m_deferred_size > 0)
    {
        m_resources.reserve(m_deferred_size);
        m_free_ranges.Add({ 0, m_deferred_size });
    }
    if (m_settings.size > 0)
    {
        Allocate();
    }
}

DescriptorHeap::~DescriptorHeap()
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_modification_mutex);

    // All descriptor ranges must be released when heap is destroyed
    assert((!m_deferred_size && m_free_ranges.IsEmpty()) ||
           m_free_ranges == RangeSet({ { 0, m_deferred_size } }));
}

Data::Index DescriptorHeap::AddResource(const Base::Resource& resource)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_modification_mutex);

    if (!m_settings.deferred_allocation)
    {
        META_CHECK_ARG_LESS_DESCR(m_resources.size(), m_settings.size + 1,
                                  "{} descriptor heap is full, no free space to add a resource",
                                  magic_enum::enum_name(m_settings.type));
    }
    else if (m_resources.size() >= m_settings.size)
    {
        m_deferred_size++;
        Allocate();
    }

    m_resources.push_back(&resource);

    const auto resource_index = static_cast<Data::Index>(m_resources.size() - 1);
    m_free_ranges.Remove(Range(resource_index, resource_index + 1));

    return static_cast<int32_t>(resource_index);
}

Data::Index DescriptorHeap::ReplaceResource(const Base::Resource& resource, Data::Index at_index)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_modification_mutex);

    META_CHECK_ARG_LESS(at_index, m_resources.size());
    m_resources[at_index] = &resource;
    return at_index;
}

void DescriptorHeap::RemoveResource(Data::Index at_index)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_modification_mutex);

    META_CHECK_ARG_LESS(at_index, m_resources.size());
    m_resources[at_index] = nullptr;
    m_free_ranges.Add(Range(at_index, at_index + 1));
}

DescriptorHeap::Range DescriptorHeap::ReserveRange(Data::Size length)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_ZERO_DESCR(length, "unable to reserve empty descriptor range");
    std::scoped_lock lock_guard(m_modification_mutex);

    if (const Range reserved_range = Data::ReserveRange(m_free_ranges, length);
        reserved_range || !m_settings.deferred_allocation)
        return reserved_range;

    Range deferred_range(m_deferred_size, m_deferred_size + length);
    m_deferred_size += length;
    return deferred_range;
}

void DescriptorHeap::ReleaseRange(const Range& range)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_modification_mutex);
    m_free_ranges.Add(range);
}

void DescriptorHeap::SetDeferredAllocation(bool deferred_allocation)
{
    META_FUNCTION_TASK();
    m_settings.deferred_allocation = deferred_allocation;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetNativeCpuDescriptorHandle(uint32_t descriptor_index) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_cp_descriptor_heap);
    META_CHECK_ARG_LESS(descriptor_index, GetAllocatedSize());
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_cp_descriptor_heap->GetCPUDescriptorHandleForHeapStart(), descriptor_index, m_descriptor_size);
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetNativeGpuDescriptorHandle(uint32_t descriptor_index) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_cp_descriptor_heap);
    META_CHECK_ARG_LESS(descriptor_index, GetAllocatedSize());
    return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_cp_descriptor_heap->GetGPUDescriptorHandleForHeapStart(), descriptor_index, m_descriptor_size);
}

void DescriptorHeap::Allocate()
{
    META_FUNCTION_TASK();
    const Data::Size allocated_size = GetAllocatedSize();
    const Data::Size deferred_size  = GetDeferredSize();

    if (allocated_size == deferred_size)
        return;

    const wrl::ComPtr<ID3D12Device>&  cp_device = m_dx_context.GetDirectDevice().GetNativeDevice();
    META_CHECK_ARG_NOT_NULL(cp_device);

    const bool is_shader_visible_heap = GetSettings().shader_visible;
    wrl::ComPtr<ID3D12DescriptorHeap> cp_old_descriptor_heap = m_cp_descriptor_heap;

    D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};
    heap_desc.NumDescriptors = deferred_size;
    heap_desc.Type           = m_descriptor_heap_type;
    heap_desc.Flags          = is_shader_visible_heap
                             ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
                             : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    // Allocate new descriptor heap of deferred size
    ThrowIfFailed(cp_device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&m_cp_descriptor_heap)), cp_device.Get());

    if (!is_shader_visible_heap && cp_old_descriptor_heap && allocated_size > 0)
    {
        // Copy descriptors from old heap to the new one. It works for non-shader-visible CPU heaps only.
        // Shader-visible heaps must be re-filled with updated descriptors
        // using Rhi::IProgramBindings::CompleteInitialization() & DescriptorManager::CompleteInitialization()
        cp_device->CopyDescriptorsSimple(allocated_size,
                                         m_cp_descriptor_heap->GetCPUDescriptorHandleForHeapStart(),
                                         cp_old_descriptor_heap->GetCPUDescriptorHandleForHeapStart(),
                                         m_descriptor_heap_type);
    }

    m_allocated_size = m_deferred_size;
    Emit(&IDescriptorHeapCallback::OnDescriptorHeapAllocated, std::ref(*this));
}

} // namespace Methane::Graphics::DirectX
