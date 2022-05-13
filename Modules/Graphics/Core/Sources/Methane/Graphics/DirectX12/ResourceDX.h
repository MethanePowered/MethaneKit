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

#include "ContextDX.h"
#include "DescriptorHeapDX.h"
#include "ResourceBarriersDX.h"

#include <Methane/Graphics/Resource.h>

#include <wrl.h>
#include <d3d12.h>

namespace Methane::Graphics
{

namespace wrl = Microsoft::WRL;

struct IResourceDX;

class ResourceLocationDX final : public ResourceLocation
{
public:
    struct Id
    {
        Resource::Usage     usage;
        Ref<const Settings> settings_ref;

        Id(Resource::Usage usage, const ResourceLocation::Settings& settings);

        [[nodiscard]] bool operator<(const Id& other) const noexcept;
        [[nodiscard]] bool operator==(const Id& other) const noexcept;
        [[nodiscard]] bool operator!=(const Id& other) const noexcept;
    };

    ResourceLocationDX(const ResourceLocation& location, Resource::Usage usage);
    ~ResourceLocationDX();

    [[nodiscard]] const Id&                        GetId() const noexcept                { return m_id; }
    [[nodiscard]] Resource::Usage                  GetUsage() const noexcept             { return m_id.usage; }
    [[nodiscard]] IResourceDX&                     GetResourceDX() const noexcept        { return m_resource_dx; }
    [[nodiscard]] const IContextDX&                GetContextDX() const noexcept         { return m_context_dx; }
    [[nodiscard]] bool                             HasDescriptor() const noexcept        { return m_descriptor_opt.has_value(); }
    [[nodiscard]] const Opt<Resource::Descriptor>& GetDescriptor() const noexcept        { return m_descriptor_opt; }
    [[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS        GetNativeGpuAddress() const noexcept;
    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE      GetNativeCpuDescriptorHandle() const noexcept;
    [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE      GetNativeGpuDescriptorHandle() const noexcept;

private:
    Id                         m_id;
    IResourceDX&               m_resource_dx;
    const IContextDX&          m_context_dx;
    Opt<Resource::Descriptor>  m_descriptor_opt;
};

using ResourceLocationsDX = std::vector<ResourceLocationDX>;

struct IResourceDX : virtual Resource // NOSONAR
{
public:
    using Barrier     = Resource::Barrier;
    using Barriers    = Resource::Barriers;
    using State       = Resource::State;
    using BarriersDX  = ResourceBarriersDX;
    using LocationDX  = ResourceLocationDX;
    using LocationsDX = ResourceLocationsDX;

    [[nodiscard]] static DescriptorHeapDX::Type GetDescriptorHeapTypeByUsage(Resource& resource, Resource::Usage resource_usage);
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
    [[nodiscard]] virtual Opt<Descriptor>                     GetNativeViewDescriptor(const LocationDX& location) = 0;

    ~IResourceDX() override = default;
};

} // namespace Methane::Graphics
