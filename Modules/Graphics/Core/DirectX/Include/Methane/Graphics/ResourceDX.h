/******************************************************************************

Copyright 2019-2022 Evgeny Gorodetskiy

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
DirectX 12 specialization of the resource interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/ContextDX.h>
#include <Methane/Graphics/DescriptorHeapDX.h>
#include <Methane/Graphics/ResourceBarriersDX.h>

#include <Methane/Graphics/IResource.h>

#include <wrl.h>
#include <directx/d3d12.h>

namespace Methane::Graphics
{

namespace wrl = Microsoft::WRL;

struct IResourceDX;

class ResourceViewDX final : public ResourceView
{
public:
    ResourceViewDX(const ResourceView& view_id, IResource::Usage usage);

    [[nodiscard]] const Id&                        GetId() const noexcept                { return m_id; }
    [[nodiscard]] IResource::Usage                  GetUsage() const noexcept             { return m_id.usage; }
    [[nodiscard]] IResourceDX&                     GetResourceDX() const noexcept        { return m_resource_dx; }
    [[nodiscard]] bool                             HasDescriptor() const noexcept        { return m_descriptor_opt.has_value(); }
    [[nodiscard]] const Opt<IResource::Descriptor>& GetDescriptor() const noexcept        { return m_descriptor_opt; }
    [[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS        GetNativeGpuAddress() const noexcept;
    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE      GetNativeCpuDescriptorHandle() const noexcept;
    [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE      GetNativeGpuDescriptorHandle() const noexcept;

private:
    Id                         m_id;
    IResourceDX&               m_resource_dx;
    Opt<IResource::Descriptor> m_descriptor_opt;
};

using ResourceViewsDX = std::vector<ResourceViewDX>;

struct IResourceDX : virtual IResource // NOSONAR
{
public:
    using Barrier     = IResource::Barrier;
    using Barriers    = IResourceBarriers;
    using State       = IResource::State;
    using BarriersDX  = ResourceBarriersDX;
    using ViewDX      = ResourceViewDX;
    using ViewsDX     = ResourceViewsDX;

    [[nodiscard]] static DescriptorHeapDX::Type GetDescriptorHeapTypeByUsage(const IResource& resource, IResource::Usage resource_usage);
    [[nodiscard]] static D3D12_RESOURCE_STATES  GetNativeResourceState(State resource_state);
    [[nodiscard]] static D3D12_RESOURCE_BARRIER GetNativeResourceBarrier(const Barrier::Id& id, const Barrier::StateChange& state_change);
    [[nodiscard]] static D3D12_RESOURCE_BARRIER GetNativeResourceBarrier(const Barrier& resource_barrier)
    {
        return GetNativeResourceBarrier(resource_barrier.GetId(), resource_barrier.GetStateChange());
    }

    [[nodiscard]] virtual ID3D12Resource&                     GetNativeResourceRef() const = 0;
    [[nodiscard]] virtual ID3D12Resource*                     GetNativeResource() const noexcept = 0;
    [[nodiscard]] virtual const wrl::ComPtr<ID3D12Resource>&  GetNativeResourceComPtr() const noexcept = 0;
    [[nodiscard]] virtual D3D12_GPU_VIRTUAL_ADDRESS           GetNativeGpuAddress() const noexcept = 0;

    virtual Opt<Descriptor> InitializeNativeViewDescriptor(const ViewDX::Id& view_id) = 0;

    ~IResourceDX() override = default;
};

} // namespace Methane::Graphics
