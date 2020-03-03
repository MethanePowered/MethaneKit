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

FILE: Methane/Graphics/DirectX12/DescriptorHeapDX.cpp
DirectX 12 implementation of the descriptor heap wrapper.

******************************************************************************/

#include "DescriptorHeapDX.h"
#include "DeviceDX.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>
#include <Methane/Graphics/Windows/Helpers.h>

#include <cassert>

namespace Methane::Graphics
{

static D3D12_DESCRIPTOR_HEAP_TYPE GetNativeHeapType(DescriptorHeap::Type type) noexcept
{
    ITT_FUNCTION_TASK();
    switch (type)
    {
    case DescriptorHeap::Type::ShaderResources: return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    case DescriptorHeap::Type::Samplers:        return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    case DescriptorHeap::Type::RenderTargets:   return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    case DescriptorHeap::Type::DepthStencil:    return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    default:                                    assert(0);
    }
    return D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
}

Ptr<DescriptorHeap> DescriptorHeap::Create(ContextBase& context, const Settings& settings)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<DescriptorHeapDX>(context, settings);
}

DescriptorHeapDX::DescriptorHeapDX(ContextBase& context, const Settings& settings)
    : DescriptorHeap(context, settings)
    , m_descriptor_heap_type(GetNativeHeapType(m_settings.type))
    , m_descriptor_size(GetContextDX().GetDeviceDX().GetNativeDevice()->GetDescriptorHandleIncrementSize(m_descriptor_heap_type))
{
    ITT_FUNCTION_TASK();
    if (m_deferred_size > 0)
    {
        DescriptorHeapDX::Allocate();
    }
}

DescriptorHeapDX::~DescriptorHeapDX()
{
    ITT_FUNCTION_TASK();
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapDX::GetNativeCpuDescriptorHandle(uint32_t descriptor_index) const noexcept
{
    ITT_FUNCTION_TASK();
    assert(!!m_cp_descriptor_heap);
    assert(descriptor_index < m_allocated_size);
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_cp_descriptor_heap->GetCPUDescriptorHandleForHeapStart(), descriptor_index, m_descriptor_size);
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapDX::GetNativeGpuDescriptorHandle(uint32_t descriptor_index) const noexcept
{
    ITT_FUNCTION_TASK();
    assert(!!m_cp_descriptor_heap);
    return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_cp_descriptor_heap->GetGPUDescriptorHandleForHeapStart(), descriptor_index, m_descriptor_size);
}

void DescriptorHeapDX::Allocate()
{
    ITT_FUNCTION_TASK();
    if (m_allocated_size == m_deferred_size)
        return;

    const wrl::ComPtr<ID3D12Device>&  cp_device = GetContextDX().GetDeviceDX().GetNativeDevice();
    assert(!!cp_device);

    wrl::ComPtr<ID3D12DescriptorHeap> cp_old_descriptor_heap = m_cp_descriptor_heap;

    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.NumDescriptors = m_deferred_size;
    heap_desc.Type           = m_descriptor_heap_type;
    heap_desc.Flags          = m_settings.shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    // Allocate new descriptor heap of deferred size
    ThrowIfFailed(cp_device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&m_cp_descriptor_heap)));

    if (cp_old_descriptor_heap && m_allocated_size > 0)
    {
        // Copy descriptors from old heap to the new one
        cp_device->CopyDescriptorsSimple(m_allocated_size,
                                         m_cp_descriptor_heap->GetCPUDescriptorHandleForHeapStart(),
                                         cp_old_descriptor_heap->GetCPUDescriptorHandleForHeapStart(),
                                         m_descriptor_heap_type);
    }

    DescriptorHeap::Allocate();
}

IContextDX& DescriptorHeapDX::GetContextDX() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<IContextDX&>(m_context);
}

} // namespace Methane::Graphics
