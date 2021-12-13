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
#include "BlitCommandListDX.h"
#include "DeviceDX.h"

#include <Methane/Graphics/TextureBase.h>
#include <Methane/Graphics/CommandKit.h>
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

template<typename ResourceBaseType, typename = std::enable_if_t<std::is_base_of_v<ResourceBase, ResourceBaseType>, void>>
class ResourceDX // NOSONAR - destructor in use
    : public ResourceBaseType
    , public IResourceDX
{
public:
    template<typename SettingsType>
    ResourceDX(const ContextBase& context, const SettingsType& settings, const DescriptorByUsage& descriptor_by_usage)
        : ResourceBaseType(context, settings)
        , m_descriptor_by_usage(descriptor_by_usage)
    {
        META_FUNCTION_TASK();
        for (const auto& [usage, descriptor] : m_descriptor_by_usage)
        {
            descriptor.heap.ReplaceResource(*this, descriptor.index);
            m_descriptor_heap_types.insert(descriptor.heap.GetSettings().type);
        }
    }

    ~ResourceDX() override
    {
        for (const auto& [usage, descriptor] : m_descriptor_by_usage)
        {
            descriptor.heap.RemoveResource(descriptor.index);
        }
        // Resource released callback has to be emitted before native resource is released
        Data::Emitter<IResourceCallback>::Emit(&IResourceCallback::OnResourceReleased, std::ref(*this));
    }

    ResourceDX(const ResourceDX&) = delete;
    ResourceDX(ResourceDX&&) = delete;

    bool operator=(const ResourceDX&) = delete;
    bool operator=(ResourceDX&&) = delete;

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

    // IResource overrides
    const DescriptorByUsage& GetDescriptorByUsage() const noexcept final { return m_descriptor_by_usage; }

    const Descriptor& GetDescriptor(Usage usage) const final
    {
        META_FUNCTION_TASK();
        auto descriptor_by_usage_it = m_descriptor_by_usage.find(usage);
        META_CHECK_ARG_DESCR(usage, descriptor_by_usage_it != m_descriptor_by_usage.end(),
                             "resource '{}' does not support '{}' usage",
                             GetName(), magic_enum::enum_name(usage));
        return descriptor_by_usage_it->second;
    }

    // IResourceDX overrides
    ID3D12Resource&                    GetNativeResourceRef() const final                                        { META_CHECK_ARG_NOT_NULL(m_cp_resource); return *m_cp_resource.Get(); }
    ID3D12Resource*                    GetNativeResource() const noexcept final                                  { return m_cp_resource.Get(); }
    const wrl::ComPtr<ID3D12Resource>& GetNativeResourceComPtr() const noexcept final                            { return m_cp_resource; }
    D3D12_GPU_VIRTUAL_ADDRESS          GetNativeGpuAddress() const noexcept final                                { return m_cp_resource ? m_cp_resource->GetGPUVirtualAddress() : 0; }
    D3D12_CPU_DESCRIPTOR_HANDLE        GetNativeCpuDescriptorHandle(Usage usage) const noexcept final            { return GetNativeCpuDescriptorHandle(GetDescriptorByUsage(usage)); }
    D3D12_CPU_DESCRIPTOR_HANDLE        GetNativeCpuDescriptorHandle(const Descriptor& desc) const noexcept final { return static_cast<const DescriptorHeapDX&>(desc.heap).GetNativeCpuDescriptorHandle(desc.index); }
    D3D12_GPU_DESCRIPTOR_HANDLE        GetNativeGpuDescriptorHandle(Usage usage) const noexcept final            { return GetNativeGpuDescriptorHandle(GetDescriptorByUsage(usage)); }
    D3D12_GPU_DESCRIPTOR_HANDLE        GetNativeGpuDescriptorHandle(const Descriptor& desc) const noexcept final { return static_cast<const DescriptorHeapDX&>(desc.heap).GetNativeGpuDescriptorHandle(desc.index); }
    const DescriptorHeapDX::Types&       GetDescriptorHeapTypes() const noexcept final                             { return m_descriptor_heap_types; }

protected:
    const IContextDX& GetContextDX() const noexcept { return static_cast<const IContextDX&>(GetContextBase()); }

    void InitializeDefaultDescriptors()
    {
        META_FUNCTION_TASK();
        const Usage usage_mask = GetUsage();
        DescriptorManagerDX& descriptor_manager = GetContextDX().GetDescriptorManagerDX();

        using namespace magic_enum::bitwise_operators;
        for (Usage usage : GetPrimaryUsageValues())
        {
            if (!magic_enum::flags::enum_contains(usage & usage_mask))
                continue;

            if (const auto descriptor_by_usage_it = m_descriptor_by_usage.find(usage);
                descriptor_by_usage_it == m_descriptor_by_usage.end())
            {
                // Create default resource descriptor by usage
                const DescriptorHeapDX::Type heap_type = GetDescriptorHeapTypeByUsage(usage);
                DescriptorHeapDX& heap = descriptor_manager.GetDescriptorHeap(heap_type);
                m_descriptor_by_usage.try_emplace(usage, Descriptor(heap, heap.AddResource(*this)));
                m_descriptor_heap_types.insert(heap.GetSettings().type);
            }
        }
    }

    DescriptorHeapDX::Type GetDescriptorHeapTypeByUsage(ResourceBase::Usage resource_usage) const
    {
        META_FUNCTION_TASK();
        switch (resource_usage)
        {
        case Resource::Usage::ShaderRead:
            return (GetResourceType() == Resource::Type::Sampler)
                 ? DescriptorHeapDX::Type::Samplers
                 : DescriptorHeapDX::Type::ShaderResources;

        case Resource::Usage::ShaderWrite:
        case Resource::Usage::RenderTarget:
            return (GetResourceType() == Resource::Type::Texture &&
                    dynamic_cast<const TextureBase&>(*this).GetSettings().type == Texture::Type::DepthStencilBuffer)
                 ? DescriptorHeapDX::Type::DepthStencil
                 : DescriptorHeapDX::Type::RenderTargets;

        default:
            META_UNEXPECTED_ARG_DESCR_RETURN(resource_usage, DescriptorHeapDX::Type::Undefined,
                                             "resource usage does not map to descriptor heap");
        }
    }

    const Resource::Descriptor& GetDescriptorByUsage(Usage usage) const
    {
        META_FUNCTION_TASK();
        auto descriptor_by_usage_it = m_descriptor_by_usage.find(usage);
        META_CHECK_ARG_DESCR(usage, descriptor_by_usage_it != m_descriptor_by_usage.end(),
                             "Resource '{}' does not have descriptor for usage '{}'",
                             GetName(), magic_enum::enum_name(usage));
        return descriptor_by_usage_it->second;
    }

    wrl::ComPtr<ID3D12Resource> CreateCommittedResource(const D3D12_RESOURCE_DESC& resource_desc, D3D12_HEAP_TYPE heap_type,
                                                        D3D12_RESOURCE_STATES resource_state, const D3D12_CLEAR_VALUE* p_clear_value = nullptr)
    {
        META_FUNCTION_TASK();
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
            static_cast<const RenderContextDX&>(GetContextDX()).GetNativeSwapChain()->GetBuffer(
                frame_buffer_index,
                IID_PPV_ARGS(&m_cp_resource)
            ),
            GetContextDX().GetDeviceDX().GetNativeDevice().Get()
        );
    }

    BlitCommandListDX& PrepareResourceUpload(CommandQueue* sync_cmd_queue)
    {
        META_FUNCTION_TASK();
        auto& upload_cmd_list = dynamic_cast<BlitCommandListDX&>(GetContext().GetUploadCommandKit().GetListForEncoding());
        upload_cmd_list.RetainResource(*this);

        // When upload command list has COPY type, before transitioning resource to CopyDest state prior copying,
        // first it has to be be transitioned to Common state with synchronization command list of DIRECT type.
        // This is required due to DX12 limitation of using only copy-related resource barrier states in command lists of COPY type.
        if (upload_cmd_list.GetNativeCommandList().GetType() == D3D12_COMMAND_LIST_TYPE_COPY &&
            SetState(State::Common, m_upload_sync_transition_barriers_ptr) && m_upload_sync_transition_barriers_ptr)
        {
            CommandList& sync_cmd_list = sync_cmd_queue
                                       ? GetContext().GetDefaultCommandKit(*sync_cmd_queue).GetListForEncoding()
                                       : upload_cmd_list;
            sync_cmd_list.SetResourceBarriers(*m_upload_sync_transition_barriers_ptr);
        }

        if (SetState(State::CopyDest, m_upload_begin_transition_barriers_ptr) && m_upload_begin_transition_barriers_ptr)
        {
            upload_cmd_list.SetResourceBarriers(*m_upload_begin_transition_barriers_ptr);
        }

        return upload_cmd_list;
    }

private:
    DescriptorByUsage           m_descriptor_by_usage;
    DescriptorHeapDX::Types     m_descriptor_heap_types;
    wrl::ComPtr<ID3D12Resource> m_cp_resource;
    Ptr<Resource::Barriers>     m_upload_sync_transition_barriers_ptr;
    Ptr<Resource::Barriers>     m_upload_begin_transition_barriers_ptr;
};

} // namespace Methane::Graphics
