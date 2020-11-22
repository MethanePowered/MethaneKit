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

FILE: Methane/Graphics/DirectX12/ResourceDX.h
DirectX 12 implementation of the resource interface.

******************************************************************************/

#pragma once

#include "ResourceDX.h"
#include "DescriptorHeapDX.h"
#include "RenderContextDX.h"
#include "DeviceDX.h"

#include <Methane/Graphics/ResourceBase.h>
#include <Methane/Graphics/Windows/ErrorHandling.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <wrl.h>
#include <d3d12.h>

namespace Methane::Graphics
{

namespace wrl = Microsoft::WRL;

struct IContextDX;
class DescriptorHeapDX;

template<typename ReourceBaseType, typename = std::enable_if_t<std::is_base_of_v<ResourceBase, ReourceBaseType>, void>>
class ResourceDX
    : public ReourceBaseType
    , public IResourceDX
{
public:
    template<typename SettingsType>
    ResourceDX(ContextBase& context, const SettingsType& settings, const DescriptorByUsage& descriptor_by_usage)
        : ReourceBaseType(context, settings, descriptor_by_usage)
    {
        META_FUNCTION_TASK();
    }

    void ForceReleaseResource() { m_cp_resource.Reset(); }

    // Object interface
    void SetName(const std::string& name) override
    {
        META_FUNCTION_TASK();
        ResourceBase::SetName(name);
        if (m_cp_resource)
        {
            m_cp_resource->SetName(nowide::widen(name).c_str());
        }
    }

    // IResourceDX overrides

    ID3D12Resource&                     GetNativeResourceRef() const final                                        { META_CHECK_ARG_NOT_NULL(m_cp_resource); return *m_cp_resource.Get(); }
    ID3D12Resource*                     GetNativeResource() const noexcept final                                  { return m_cp_resource.Get(); }
    const wrl::ComPtr<ID3D12Resource>&  GetNativeResourceComPtr() const noexcept final                            { return m_cp_resource; }
    D3D12_GPU_VIRTUAL_ADDRESS           GetNativeGpuAddress() const noexcept final                                { return m_cp_resource ? m_cp_resource->GetGPUVirtualAddress() : 0; }
    D3D12_CPU_DESCRIPTOR_HANDLE         GetNativeCpuDescriptorHandle(Usage usage) const noexcept final     { return GetNativeCpuDescriptorHandle(GetDescriptorByUsage(usage)); }
    D3D12_CPU_DESCRIPTOR_HANDLE         GetNativeCpuDescriptorHandle(const Descriptor& desc) const noexcept final { return static_cast<const DescriptorHeapDX&>(desc.heap).GetNativeCpuDescriptorHandle(desc.index); }
    D3D12_GPU_DESCRIPTOR_HANDLE         GetNativeGpuDescriptorHandle(Usage usage) const noexcept final     { return GetNativeGpuDescriptorHandle(GetDescriptorByUsage(usage)); }
    D3D12_GPU_DESCRIPTOR_HANDLE         GetNativeGpuDescriptorHandle(const Descriptor& desc) const noexcept final { return static_cast<const DescriptorHeapDX&>(desc.heap).GetNativeGpuDescriptorHandle(desc.index); }
    DescriptorHeap::Types               GetDescriptorHeapTypes() const noexcept final                             { return GetUsedDescriptorHeapTypes(); }

protected:
    IContextDX& GetContextDX() noexcept { return static_cast<IContextDX&>(GetContextBase()); }

    wrl::ComPtr<ID3D12Resource> CreateCommittedResource(const D3D12_RESOURCE_DESC& resource_desc, D3D12_HEAP_TYPE heap_type,
                                                        D3D12_RESOURCE_STATES resource_state, const D3D12_CLEAR_VALUE* p_clear_value = nullptr)
    {
        wrl::ComPtr<ID3D12Resource> cp_resource;
        const CD3DX12_HEAP_PROPERTIES heap_properties(heap_type);
        const wrl::ComPtr<ID3D12Device>& cp_native_device = GetContextDX().GetDeviceDX().GetNativeDevice();
        ThrowIfFailed(
            cp_native_device->CreateCommittedResource(
                &heap_properties,
                D3D12_HEAP_FLAG_NONE,
                &resource_desc,
                resource_state,
                p_clear_value,
                IID_PPV_ARGS(&cp_resource)
            ),
            cp_native_device.Get()
        );
        return cp_resource;
    }

    void InitializeCommittedResource(const D3D12_RESOURCE_DESC& resource_desc, D3D12_HEAP_TYPE heap_type,
                                     D3D12_RESOURCE_STATES resource_state, const D3D12_CLEAR_VALUE* p_clear_value = nullptr)
    {
        META_FUNCTION_TASK();
        META_CHECK_ARG_DESCR(m_cp_resource, !m_cp_resource, "committed resource is already initialized");
        m_cp_resource = CreateCommittedResource(resource_desc, heap_type, resource_state, p_clear_value);
    }

    void InitializeFrameBufferResource(uint32_t frame_buffer_index)
    {
        META_FUNCTION_TASK();
        META_CHECK_ARG_DESCR(m_cp_resource, !m_cp_resource, "committed resource is already initialized");

        ThrowIfFailed(
            static_cast<RenderContextDX&>(GetContextDX()).GetNativeSwapChain()->GetBuffer(
                frame_buffer_index,
                IID_PPV_ARGS(&m_cp_resource)
            ),
            GetContextDX().GetDeviceDX().GetNativeDevice().Get()
        );
    }

private:
    wrl::ComPtr<ID3D12Resource> m_cp_resource;
};

} // namespace Methane::Graphics
