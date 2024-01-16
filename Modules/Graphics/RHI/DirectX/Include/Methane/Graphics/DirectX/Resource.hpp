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

FILE: Methane/Graphics/DirectX/Resource.h
DirectX 12 implementation of the resource interface.

******************************************************************************/

#pragma once

#include "IResource.h"
#include "DescriptorHeap.h"
#include "TransferCommandList.h"
#include "Device.h"
#include "DescriptorManager.h"
#include "ErrorHandling.h"

#include <Methane/Graphics/Base/Texture.h>
#include <Methane/Graphics/RHI/ICommandKit.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <wrl.h>
#include <directx/d3d12.h>
#include <cassert>

namespace Methane::Graphics::DirectX
{

namespace wrl = Microsoft::WRL;

struct IContext;
class DescriptorHeap;

template<typename ResourceBaseType, typename = std::enable_if_t<std::is_base_of_v<Base::Resource, ResourceBaseType>, void>>
class Resource // NOSONAR - destructor in use
    : public ResourceBaseType
    , public IResource
{
public:
    template<typename SettingsType>
    Resource(const Base::Context& context, const SettingsType& settings)
        : ResourceBaseType(context, settings, State::Common, State::Common)
        , m_dx_context(dynamic_cast<const IContext&>(context))
    { }

    ~Resource() override
    {
        for (const auto& [view_id, descriptor] : m_descriptor_by_view_id)
        {
            descriptor.heap.RemoveResource(descriptor.index);
        }

        try
        {
            // Resource released callback has to be emitted before native resource is released
            Data::Emitter<Rhi::IResourceCallback>::Emit(&Rhi::IResourceCallback::OnResourceReleased, std::ref(*this));
        }
        catch(const std::exception& e)
        {
            META_UNUSED(e);
            META_LOG("WARNING: Unexpected error during resource destruction: {}", e.what());
            assert(false);
        }
    }

    Resource(const Resource&) = delete;
    Resource(Resource&&) = delete;

    bool operator=(const Resource&) = delete;
    bool operator=(Resource&&) = delete;

    // IObject interface
    bool SetName(std::string_view name) override
    {
        META_FUNCTION_TASK();
        if (!Base::Resource::SetName(name))
            return false;

        if (m_cp_resource)
        {
            m_cp_resource->SetName(nowide::widen(name).c_str());
        }
        return true;
    }

    const DescriptorByViewId& GetDescriptorByViewId() const noexcept final { return m_descriptor_by_view_id; }

    void RestoreDescriptorViews(const DescriptorByViewId& descriptor_by_view_id) final
    {
        META_FUNCTION_TASK();
        META_CHECK_ARG_TRUE_DESCR(m_descriptor_by_view_id.empty(), "can not restore on resource with non-empty descriptor by view_id");
        m_descriptor_by_view_id = descriptor_by_view_id;
        for (const auto& [view_id, descriptor] : m_descriptor_by_view_id)
        {
            descriptor.heap.ReplaceResource(*this, descriptor.index);
            InitializeNativeViewDescriptor(view_id);
        }
    }

    // IResource overrides
    ID3D12Resource&                    GetNativeResourceRef() const final             { META_CHECK_ARG_NOT_NULL(m_cp_resource); return *m_cp_resource.Get(); }
    ID3D12Resource*                    GetNativeResource() const noexcept final       { return m_cp_resource.Get(); }
    const wrl::ComPtr<ID3D12Resource>& GetNativeResourceComPtr() const noexcept final { return m_cp_resource; }
    D3D12_GPU_VIRTUAL_ADDRESS          GetNativeGpuAddress() const noexcept final     { return m_cp_resource ? m_cp_resource->GetGPUVirtualAddress() : 0; }

protected:
    enum class TransferOperation
    {
        Upload,
        Readback
    };

    struct TransferBarriers
    {
        Ptr<Rhi::IResourceBarriers> sync_barriers_ptr;
        Ptr<Rhi::IResourceBarriers> begin_barriers_ptr;
    };

    const IContext& GetDirectContext() const noexcept { return m_dx_context; }
    void SetNativeResourceComPtr(const wrl::ComPtr<ID3D12Resource>& cp_resource) noexcept { m_cp_resource = cp_resource; }

    wrl::ComPtr<ID3D12Resource> CreateCommittedResource(const D3D12_RESOURCE_DESC& resource_desc, D3D12_HEAP_TYPE heap_type,
                                                        D3D12_RESOURCE_STATES resource_state, const D3D12_CLEAR_VALUE* p_clear_value = nullptr)
    {
        META_FUNCTION_TASK();
        wrl::ComPtr<ID3D12Resource> cp_resource;
        const CD3DX12_HEAP_PROPERTIES heap_properties(heap_type);
        const wrl::ComPtr<ID3D12Device>& cp_native_device = GetDirectContext().GetDirectDevice().GetNativeDevice();
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
                                     Rhi::ResourceState resource_state, const D3D12_CLEAR_VALUE* p_clear_value = nullptr)
    {
        META_FUNCTION_TASK();
        META_CHECK_ARG_DESCR(m_cp_resource.Get(), !m_cp_resource, "committed resource is already initialized");
        const D3D12_RESOURCE_STATES d3d_resource_state = IResource::GetNativeResourceState(resource_state);
        m_cp_resource = CreateCommittedResource(resource_desc, heap_type, d3d_resource_state, p_clear_value);
        SetState(resource_state);
    }

    TransferCommandList& PrepareResourceTransfer(TransferOperation transfer_operation, Rhi::ICommandQueue& target_cmd_queue, State transfer_state)
    {
        META_FUNCTION_TASK();
        auto& transfer_cmd_list = dynamic_cast<TransferCommandList&>(GetContext().GetUploadCommandKit().GetListForEncoding());
        if (GetState() == transfer_state)
            return transfer_cmd_list;

        TransferBarriers& transfer_barriers = transfer_operation == TransferOperation::Upload ? m_upload_barriers : m_read_back_barriers;
        transfer_cmd_list.RetainResource(*this);

        // When upload command list has COPY type, before transitioning resource to CopyDest state prior copying,
        // first it has to be transitioned to Common state with synchronization command list of DIRECT type.
        // This is required due to DX12 limitation of using only copy-related resource barrier states in command lists of COPY type.
        if (transfer_cmd_list.GetNativeCommandList().GetType() == D3D12_COMMAND_LIST_TYPE_COPY &&
            SetState(State::Common, transfer_barriers.sync_barriers_ptr) && transfer_barriers.sync_barriers_ptr)
        {
            Rhi::ICommandList& sync_cmd_list = GetContext().GetDefaultCommandKit(target_cmd_queue).GetListForEncoding(
                static_cast<Rhi::CommandListId>(Rhi::CommandListPurpose::PreUploadSync));
            sync_cmd_list.SetResourceBarriers(*transfer_barriers.sync_barriers_ptr);
        }

        if (SetState(transfer_state, transfer_barriers.begin_barriers_ptr) && transfer_barriers.begin_barriers_ptr)
        {
            transfer_cmd_list.SetResourceBarriers(*transfer_barriers.begin_barriers_ptr);
        }

        return transfer_cmd_list;
    }

    const IResource::Descriptor& GetDescriptorByViewId(const ResourceView::Id& view_id)
    {
        META_FUNCTION_TASK();
        if (const auto it = m_descriptor_by_view_id.find(view_id);
            it != m_descriptor_by_view_id.end())
            return it->second;

        return m_descriptor_by_view_id.try_emplace(view_id, CreateResourceDescriptor(view_id.usage)).first->second;
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
    Rhi::IResource::Descriptor CreateResourceDescriptor(UsageMask usage)
    {
        META_FUNCTION_TASK();
        DescriptorManager& descriptor_manager = GetDirectContext().GetDirectDescriptorManager();
        const DescriptorHeap::Type heap_type = IResource::GetDescriptorHeapTypeByUsage(*this, usage);
        DescriptorHeap& heap = descriptor_manager.GetDescriptorHeap(heap_type);
        return IResource::Descriptor(heap, heap.AddResource(dynamic_cast<Base::Resource&>(*this)));
    }

    const IContext&             m_dx_context;
    DescriptorByViewId          m_descriptor_by_view_id;
    wrl::ComPtr<ID3D12Resource> m_cp_resource;
    TransferBarriers            m_upload_barriers;
    TransferBarriers            m_read_back_barriers;
};

} // namespace Methane::Graphics::DirectX
