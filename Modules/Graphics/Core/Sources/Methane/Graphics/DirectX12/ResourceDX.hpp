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
    ResourceDX(const ContextBase& context, const SettingsType& settings)
        : ResourceBaseType(context, settings, State::Common, State::Common)
    {
        META_FUNCTION_TASK();
    }

    ~ResourceDX() override
    {
        for (const auto& [location_id, descriptor] : m_descriptor_by_location_id)
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

    // Object interface
    bool SetName(const std::string& name) override
    {
        META_FUNCTION_TASK();
        if (!ResourceBase::SetName(name))
            return false;

        if (m_cp_resource)
        {
            m_cp_resource->SetName(nowide::widen(name).c_str());
        }
        return true;
    }

    const DescriptorByLocationId& GetDescriptorByLocationId() const noexcept final { return m_descriptor_by_location_id; }

    void RestoreDescriptorLocations(const DescriptorByLocationId& descriptor_by_location_id) final
    {
        META_FUNCTION_TASK();
        META_CHECK_ARG_TRUE_DESCR(m_descriptor_by_location_id.empty(), "can not restore on resource with non-empty descriptor by location");
        m_descriptor_by_location_id = descriptor_by_location_id;
        for (const auto& [location_id, descriptor] : m_descriptor_by_location_id)
        {
            descriptor.heap.ReplaceResource(*this, descriptor.index);
            InitializeNativeViewDescriptor(location_id);
        }
    }

    // IResourceDX overrides
    ID3D12Resource&                    GetNativeResourceRef() const final                                        { META_CHECK_ARG_NOT_NULL(m_cp_resource); return *m_cp_resource.Get(); }
    ID3D12Resource*                    GetNativeResource() const noexcept final                                  { return m_cp_resource.Get(); }
    const wrl::ComPtr<ID3D12Resource>& GetNativeResourceComPtr() const noexcept final                            { return m_cp_resource; }
    D3D12_GPU_VIRTUAL_ADDRESS          GetNativeGpuAddress() const noexcept final                                { return m_cp_resource ? m_cp_resource->GetGPUVirtualAddress() : 0; }

protected:
    const IContextDX& GetContextDX() const noexcept { return static_cast<const IContextDX&>(GetContextBase()); }

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
                                     Resource::State resource_state, const D3D12_CLEAR_VALUE* p_clear_value = nullptr)
    {
        META_FUNCTION_TASK();
        META_CHECK_ARG_DESCR(m_cp_resource, !m_cp_resource, "committed resource is already initialized");
        const D3D12_RESOURCE_STATES d3d_resource_state = ResourceBarriersDX::GetNativeResourceState(resource_state);
        m_cp_resource = CreateCommittedResource(resource_desc, heap_type, d3d_resource_state, p_clear_value);
        SetState(resource_state);
    }

    void InitializeFrameBufferResource(uint32_t frame_buffer_index)
    {
        META_FUNCTION_TASK();
        META_CHECK_ARG_DESCR(m_cp_resource, !m_cp_resource, "committed resource is already initialized");
        ThrowIfFailed(
            static_cast<const RenderContextDX&>(GetContextDX()).GetNativeSwapChain()->GetBuffer(frame_buffer_index, IID_PPV_ARGS(&m_cp_resource)),
            GetContextDX().GetDeviceDX().GetNativeDevice().Get()
        );
    }

    BlitCommandListDX& PrepareResourceUpload(CommandQueue& target_cmd_queue)
    {
        META_FUNCTION_TASK();
        auto& upload_cmd_list = dynamic_cast<BlitCommandListDX&>(GetContext().GetUploadCommandKit().GetListForEncoding());
        upload_cmd_list.RetainResource(*this);

        // When upload command list has COPY type, before transitioning resource to CopyDest state prior copying,
        // first it has to be transitioned to Common state with synchronization command list of DIRECT type.
        // This is required due to DX12 limitation of using only copy-related resource barrier states in command lists of COPY type.
        if (upload_cmd_list.GetNativeCommandList().GetType() == D3D12_COMMAND_LIST_TYPE_COPY &&
            SetState(State::Common, m_upload_sync_transition_barriers_ptr) && m_upload_sync_transition_barriers_ptr)
        {
            CommandList& sync_cmd_list = GetContext().GetDefaultCommandKit(target_cmd_queue).GetListForEncoding(
                static_cast<CommandKit::CommandListId>(CommandKit::CommandListPurpose::PreUploadSync));
            sync_cmd_list.SetResourceBarriers(*m_upload_sync_transition_barriers_ptr);
        }

        if (SetState(State::CopyDest, m_upload_begin_transition_barriers_ptr) && m_upload_begin_transition_barriers_ptr)
        {
            upload_cmd_list.SetResourceBarriers(*m_upload_begin_transition_barriers_ptr);
        }

        return upload_cmd_list;
    }

protected:
    const Resource::Descriptor& GetDescriptorByLocationId(const ResourceLocationDX::Id& location_id)
    {
        META_FUNCTION_TASK();
        const auto it = m_descriptor_by_location_id.find(location_id);
        if (it != m_descriptor_by_location_id.end())
            return it->second;

        return m_descriptor_by_location_id.try_emplace(location_id, CreateResourceDescriptor(location_id.usage)).first->second;
    }

    static D3D12_CPU_DESCRIPTOR_HANDLE GetNativeCpuDescriptorHandle(const Descriptor& descriptor)
    {
        return descriptor.heap.GetNativeCpuDescriptorHandle(descriptor.index);
    }

    static D3D12_GPU_DESCRIPTOR_HANDLE GetNativeGpuDescriptorHandle(const Descriptor& descriptor)
    {
        return descriptor.heap.GetNativeGpuDescriptorHandle(descriptor.index);
    }

private:
    Resource::Descriptor CreateResourceDescriptor(Usage usage)
    {
        META_FUNCTION_TASK();
        DescriptorManagerDX& descriptor_manager = GetContextDX().GetDescriptorManagerDX();
        const DescriptorHeapDX::Type heap_type = IResourceDX::GetDescriptorHeapTypeByUsage(*this, usage);
        DescriptorHeapDX& heap = descriptor_manager.GetDescriptorHeap(heap_type);
        return Resource::Descriptor(heap, heap.AddResource(dynamic_cast<ResourceBase&>(*this)));
    }

    DescriptorByLocationId      m_descriptor_by_location_id;
    wrl::ComPtr<ID3D12Resource> m_cp_resource;
    Ptr<Resource::Barriers>     m_upload_sync_transition_barriers_ptr;
    Ptr<Resource::Barriers>     m_upload_begin_transition_barriers_ptr;
};

} // namespace Methane::Graphics
