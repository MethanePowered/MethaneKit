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

FILE: Methane/Graphics/DirectX12/ResourceBarriersDX.h
DirectX 12 specialization of the resource barriers.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Resource.h>
#include <Methane/Data/Receiver.hpp>

#include <wrl.h>
#include <d3d12.h>

namespace Methane::Graphics
{

class ResourceBarriersDX final
    : public ResourceBarriers
    , private Data::Receiver<IResourceCallback>
{
public:
    [[nodiscard]] static D3D12_RESOURCE_STATES GetNativeResourceState(ResourceState resource_state);
    [[nodiscard]] static D3D12_RESOURCE_BARRIER GetNativeResourceBarrier(const ResourceBarrier::Id& id, const ResourceBarrier::StateChange& state_change);

    explicit ResourceBarriersDX(const Set& barriers);

    // ResourceBarriers overrides
    AddResult Add(const ResourceBarrier::Id& id, const ResourceBarrier& barrier) override;
    bool Remove(const ResourceBarrier::Id& id) override;

    [[nodiscard]] const std::vector <D3D12_RESOURCE_BARRIER>& GetNativeResourceBarriers() const
    { return m_native_resource_barriers; }

private:
    // IResourceCallback
    void OnResourceReleased(Resource& resource) override;

    void AddNativeResourceBarrier(const ResourceBarrier::Id& id, const ResourceBarrier::StateChange& state_change);
    void UpdateNativeResourceBarrier(const ResourceBarrier::Id& id, const ResourceBarrier::StateChange& state_change);

    std::vector<D3D12_RESOURCE_BARRIER> m_native_resource_barriers;
};

} // namespace Methane::Graphics