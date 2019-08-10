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

FILE: Methane/Graphics/DirectX12/ResourceDX.h
DirectX 12 implementation of the resource interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/ResourceBase.h>

#include <wrl.h>
#include <d3d12.h>

namespace Methane
{
namespace Graphics
{

namespace wrl = Microsoft::WRL;

class ContextDX;
class DescriptorHeapDX;

class ResourceDX : public ResourceBase
{
public:
    using Ptr = std::shared_ptr<ResourceDX>;

    class ReleasePoolDX : public ResourceBase::ReleasePool
    {
    public:
        ReleasePoolDX() = default;

        void AddResource(ResourceBase& resource) override;
        void ReleaseResources() override;

    private:
        std::vector<wrl::ComPtr<ID3D12Resource>> m_resources;
    };

    ResourceDX(Type type, Usage::Mask usage_mask, ContextBase& context, const DescriptorByUsage& descriptor_by_usage);
    ~ResourceDX() override;

    // Object interface
    void SetName(const std::string& name) override;

    ID3D12Resource*             GetNativeResource() const noexcept                              { return m_cp_resource.Get(); }
    D3D12_GPU_VIRTUAL_ADDRESS   GetNativeGpuAddress() const noexcept                            { return m_cp_resource->GetGPUVirtualAddress(); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetNativeCPUDescriptorHandle(Usage::Value usage) const noexcept { return GetNativeCPUDescriptorHandle(GetDescriptorByUsage(usage)); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetNativeCPUDescriptorHandle(const Descriptor& desc) const noexcept;
    D3D12_GPU_DESCRIPTOR_HANDLE GetNativeGPUDescriptorHandle(Usage::Value usage) const noexcept { return GetNativeGPUDescriptorHandle(GetDescriptorByUsage(usage)); }
    D3D12_GPU_DESCRIPTOR_HANDLE GetNativeGPUDescriptorHandle(const Descriptor& desc) const noexcept;

    static D3D12_RESOURCE_STATES  GetNativeResourceState(State resource_state) noexcept;
    static D3D12_RESOURCE_BARRIER GetNativeResourceBarrier(const Barrier& resource_barrier) noexcept;

protected:
    ContextDX& GetContextDX() noexcept;

    void InitializeCommittedResource(const D3D12_RESOURCE_DESC& resource_desc, D3D12_HEAP_TYPE heap_type,
                                     D3D12_RESOURCE_STATES resource_state, const D3D12_CLEAR_VALUE* p_clear_value = nullptr);
    void InitializeFrameBufferResource(uint32_t frame_buffer_index);

    wrl::ComPtr<ID3D12Resource> m_cp_resource;
};

} // namespace Graphics
} // namespace Methane
