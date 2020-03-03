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

FILE: Methane/Graphics/DirectX12/DescriptorHeapDX.h
DirectX 12 implementation of the descriptor heap wrapper.

******************************************************************************/

#pragma once

#include <Methane/Graphics/DescriptorHeap.h>

#include <wrl.h>
#include <d3d12.h>

namespace Methane::Graphics
{

namespace wrl = Microsoft::WRL;

struct IContextDX;

class DescriptorHeapDX final : public DescriptorHeap
{
public:
    DescriptorHeapDX(ContextBase& context, const Settings& settings);
    ~DescriptorHeapDX() override;

    ID3D12DescriptorHeap*       GetNativeDescriptorHeap() noexcept           { return m_cp_descriptor_heap.Get();  }
    D3D12_DESCRIPTOR_HEAP_TYPE  GetNativeDescriptorHeapType() const noexcept { return m_descriptor_heap_type; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetNativeCpuDescriptorHandle(uint32_t descriptor_index) const noexcept;
    D3D12_GPU_DESCRIPTOR_HANDLE GetNativeGpuDescriptorHandle(uint32_t descriptor_index) const noexcept;

    // DescriptorHeap interface
    void Allocate() override;

protected:
    IContextDX& GetContextDX() noexcept;

private:
    D3D12_DESCRIPTOR_HEAP_TYPE        m_descriptor_heap_type;
    uint32_t                          m_descriptor_size;
    wrl::ComPtr<ID3D12DescriptorHeap> m_cp_descriptor_heap;
};

} // namespace Methane::Graphics
