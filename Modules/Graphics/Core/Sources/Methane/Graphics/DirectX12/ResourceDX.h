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

#include "DescriptorHeapDX.h"
#include "ResourceBarriersDX.h"

#include <Methane/Graphics/ResourceBase.h>

#include <wrl.h>
#include <d3d12.h>

namespace Methane::Graphics
{

namespace wrl = Microsoft::WRL;

struct IResourceDX : virtual Resource // NOSONAR
{
public:
    using Barrier    = Resource::Barrier;
    using Barriers   = Resource::Barriers;
    using State      = Resource::State;
    using BarriersDX = ResourceBarriersDX;

    class LocationDX final : public Location
    {
    public:
        explicit LocationDX(const Location& location)
            : Location(location)
            , m_resource_dx(dynamic_cast<IResourceDX&>(GetResource()))
        { }

        [[nodiscard]] IResourceDX&              GetResourceDX() const noexcept       { return m_resource_dx.get(); }
        [[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetNativeGpuAddress() const noexcept { return GetResourceDX().GetNativeGpuAddress() + GetOffset(); }

    private:
        Ref<IResourceDX> m_resource_dx;
    };

    using LocationsDX = std::vector<LocationDX>;

    [[nodiscard]] static D3D12_RESOURCE_STATES  GetNativeResourceState(State resource_state);
    [[nodiscard]] static D3D12_RESOURCE_BARRIER GetNativeResourceBarrier(const Barrier& resource_barrier)  { return GetNativeResourceBarrier(resource_barrier.GetId(), resource_barrier.GetStateChange()); }
    [[nodiscard]] static D3D12_RESOURCE_BARRIER GetNativeResourceBarrier(const Barrier::Id& id, const Barrier::StateChange& state_change);

    [[nodiscard]] virtual ID3D12Resource&                     GetNativeResourceRef() const = 0;
    [[nodiscard]] virtual ID3D12Resource*                     GetNativeResource() const noexcept = 0;
    [[nodiscard]] virtual const wrl::ComPtr<ID3D12Resource>&  GetNativeResourceComPtr() const noexcept = 0;
    [[nodiscard]] virtual D3D12_GPU_VIRTUAL_ADDRESS           GetNativeGpuAddress() const noexcept = 0;
    [[nodiscard]] virtual D3D12_CPU_DESCRIPTOR_HANDLE         GetNativeCpuDescriptorHandle(Usage usage) const noexcept = 0;
    [[nodiscard]] virtual D3D12_CPU_DESCRIPTOR_HANDLE         GetNativeCpuDescriptorHandle(const Descriptor& desc) const noexcept = 0;
    [[nodiscard]] virtual D3D12_GPU_DESCRIPTOR_HANDLE         GetNativeGpuDescriptorHandle(Usage usage) const noexcept = 0;
    [[nodiscard]] virtual D3D12_GPU_DESCRIPTOR_HANDLE         GetNativeGpuDescriptorHandle(const Descriptor& desc) const noexcept = 0;
    [[nodiscard]] virtual const DescriptorHeapDX::Types&      GetDescriptorHeapTypes() const noexcept = 0;

    ~IResourceDX() override = default;
};

} // namespace Methane::Graphics
