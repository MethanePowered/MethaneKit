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

FILE: Methane/Graphics/DirectX12/ResourceDX.h
DirectX 12 implementation of the resource interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/ResourceBase.h>

#include <wrl.h>
#include <d3d12.h>
#include "Methane/Data/Types.h"

namespace Methane::Graphics
{

namespace wrl = Microsoft::WRL;

struct IContextDX;
class DescriptorHeapDX;

class ResourceDX : public ResourceBase
{
public:
    class ReleasePoolDX final : public ReleasePool
    {
    public:
        ReleasePoolDX() = default;

        void AddResource(ResourceBase& resource) override;
        void ReleaseResources() override;

    private:
        std::vector<wrl::ComPtr<ID3D12Resource>> m_resources;
    };

    class LocationDX : public Location
    {
    public:
        explicit LocationDX(const Location& location);
        explicit LocationDX(const LocationDX& location);

        LocationDX& operator=(const LocationDX& other);

        ResourceDX&               GetResourceDX() const noexcept       { return m_resource_dx.get(); }
        D3D12_GPU_VIRTUAL_ADDRESS GetNativeGpuAddress() const noexcept { return GetResourceDX().GetNativeGpuAddress() + GetOffset(); }

    private:
        Ref<ResourceDX> m_resource_dx;
    };

    using LocationsDX = std::vector<LocationDX>;

    ResourceDX(Type type, Usage::Mask usage_mask, ContextBase& context, const DescriptorByUsage& descriptor_by_usage);
    ~ResourceDX() override;

    // Object interface
    void SetName(const std::string& name) override;

    ID3D12Resource&                     GetNativeResourceRef() const;
    ID3D12Resource*                     GetNativeResource() const noexcept                                  { return m_cp_resource.Get(); }
    const wrl::ComPtr<ID3D12Resource>&  GetNativeResourceComPtr() const noexcept                            { return m_cp_resource; }
    D3D12_GPU_VIRTUAL_ADDRESS           GetNativeGpuAddress() const noexcept                                { return m_cp_resource ? m_cp_resource->GetGPUVirtualAddress() : 0; }
    D3D12_CPU_DESCRIPTOR_HANDLE         GetNativeCpuDescriptorHandle(Usage::Value usage) const noexcept     { return GetNativeCpuDescriptorHandle(GetDescriptorByUsage(usage)); }
    D3D12_CPU_DESCRIPTOR_HANDLE         GetNativeCpuDescriptorHandle(const Descriptor& desc) const noexcept;
    D3D12_GPU_DESCRIPTOR_HANDLE         GetNativeGpuDescriptorHandle(Usage::Value usage) const noexcept     { return GetNativeGpuDescriptorHandle(GetDescriptorByUsage(usage)); }
    D3D12_GPU_DESCRIPTOR_HANDLE         GetNativeGpuDescriptorHandle(const Descriptor& desc) const noexcept;

    static D3D12_RESOURCE_STATES        GetNativeResourceState(State resource_state) noexcept;
    static D3D12_RESOURCE_BARRIER       GetNativeResourceBarrier(const Barrier& resource_barrier) noexcept;

protected:
    IContextDX& GetContextDX() noexcept;

    void InitializeCommittedResource(const D3D12_RESOURCE_DESC& resource_desc, D3D12_HEAP_TYPE heap_type,
                                     D3D12_RESOURCE_STATES resource_state, const D3D12_CLEAR_VALUE* p_clear_value = nullptr);
    void InitializeFrameBufferResource(uint32_t frame_buffer_index);

private:
    wrl::ComPtr<ID3D12Resource> m_cp_resource;
};

} // namespace Methane::Graphics
