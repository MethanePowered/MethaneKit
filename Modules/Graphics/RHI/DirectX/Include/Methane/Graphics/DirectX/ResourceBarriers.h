/******************************************************************************

Copyright 2020-2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/DirectX/ResourceBarriers.h
DirectX 12 specialization of the resource barriers.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/ResourceBarriers.h>
#include <Methane/Graphics/RHI/IResource.h>
#include <Methane/Data/Receiver.hpp>

#include <wrl.h>
#include <directx/d3d12.h>

namespace Methane::Graphics::DirectX
{

class ResourceBarriers final
    : public Base::ResourceBarriers
    , private Data::Receiver<Rhi::IResourceCallback>
{
public:
    [[nodiscard]] static D3D12_RESOURCE_BARRIER GetNativeResourceBarrier(const Barrier::Id& id, const Barrier::StateChange& state_change);
    [[nodiscard]] static D3D12_RESOURCE_BARRIER GetNativeResourceBarrier(const Barrier& resource_barrier)
    {
        return GetNativeResourceBarrier(resource_barrier.GetId(), resource_barrier.GetStateChange());
    }

    explicit ResourceBarriers(const Set& barriers);

    // IResourceBarriers overrides
    AddResult Add(const Barrier::Id& id, const Barrier& barrier) override;
    bool Remove(const Barrier::Id& id) override;

    [[nodiscard]] const std::vector <D3D12_RESOURCE_BARRIER>& GetNativeResourceBarriers() const
    { return m_native_resource_barriers; }

private:
    // IResourceCallback
    void OnResourceReleased(Rhi::IResource& resource) override;

    void AddNativeResourceBarrier(const Barrier::Id& id, const Barrier::StateChange& state_change);
    void UpdateNativeResourceBarrier(const Barrier::Id& id, const Barrier::StateChange& state_change);

    std::vector<D3D12_RESOURCE_BARRIER> m_native_resource_barriers;
};

} // namespace Methane::Graphics