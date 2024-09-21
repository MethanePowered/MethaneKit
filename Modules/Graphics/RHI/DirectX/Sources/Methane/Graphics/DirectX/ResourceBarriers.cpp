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

FILE: Methane/Graphics/DirectX/ResourceBarriers.cpp
DirectX 12 specialization of the resource barriers.

******************************************************************************/

#include <Methane/Graphics/DirectX/ResourceBarriers.h>
#include <Methane/Graphics/DirectX/IResource.h>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <directx/d3dx12_barriers.h>

namespace Methane::Graphics::Rhi
{

Ptr<IResourceBarriers> IResourceBarriers::Create(const Set& barriers)
{
    META_FUNCTION_TASK();
    return std::make_shared<DirectX::ResourceBarriers>(barriers);
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::DirectX
{

[[nodiscard]]
static D3D12_RESOURCE_BARRIER_TYPE GetNativeBarrierType(Rhi::ResourceBarrier::Type barrier_type)
{
    META_FUNCTION_TASK();
    switch (barrier_type) // NOSONAR
    {
    case Rhi::ResourceBarrier::Type::StateTransition: return D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    default: META_UNEXPECTED_RETURN(barrier_type, D3D12_RESOURCE_BARRIER_TYPE_TRANSITION);
    }
}

[[nodiscard]]
static std::function<bool(const D3D12_RESOURCE_BARRIER&)> GetNativeResourceBarrierPredicate(D3D12_RESOURCE_BARRIER_TYPE native_barrier_type,
                                                                                            const ID3D12Resource* native_resource_ptr)
{
    META_FUNCTION_TASK();
    switch (native_barrier_type)
    {
    case D3D12_RESOURCE_BARRIER_TYPE_TRANSITION:
        return [native_resource_ptr](const D3D12_RESOURCE_BARRIER& native_resource_barrier)
            {
                return native_resource_barrier.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION &&
                       native_resource_barrier.Transition.pResource == native_resource_ptr;
            };
    case D3D12_RESOURCE_BARRIER_TYPE_UAV:
        return [native_resource_ptr](const D3D12_RESOURCE_BARRIER& native_resource_barrier)
            {
                return native_resource_barrier.Type == D3D12_RESOURCE_BARRIER_TYPE_UAV &&
                       native_resource_barrier.UAV.pResource == native_resource_ptr;
            };
    case D3D12_RESOURCE_BARRIER_TYPE_ALIASING:
        return [native_resource_ptr](const D3D12_RESOURCE_BARRIER& native_resource_barrier)
            {
                return native_resource_barrier.Type == D3D12_RESOURCE_BARRIER_TYPE_ALIASING &&
                       native_resource_barrier.Aliasing.pResourceBefore == native_resource_ptr;
            };
    default:
        META_UNEXPECTED_RETURN(native_barrier_type, nullptr);
    }
}

D3D12_RESOURCE_BARRIER ResourceBarriers::GetNativeResourceBarrier(const Barrier::Id& id, const Barrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    switch (id.GetType()) // NOSONAR
    {
    case Barrier::Type::StateTransition:
        return CD3DX12_RESOURCE_BARRIER::Transition(
            dynamic_cast<const IResource&>(id.GetResource()).GetNativeResource(),
            IResource::GetNativeResourceState(state_change.GetStateBefore()),
            IResource::GetNativeResourceState(state_change.GetStateAfter())
        );

    default:
        META_UNEXPECTED_RETURN(id.GetType(), D3D12_RESOURCE_BARRIER());
    }
}

ResourceBarriers::ResourceBarriers(const Set& barriers)
    : Base::ResourceBarriers(barriers)
{
    META_FUNCTION_TASK();
    for(const Barrier barrier : barriers)
    {
        AddNativeResourceBarrier(barrier.GetId(), barrier.GetStateChange());
    }
}

Base::ResourceBarriers::AddResult ResourceBarriers::Add(const Barrier::Id& id, const Barrier& barrier)
{
    META_FUNCTION_TASK();
    const auto lock_guard  = Base::ResourceBarriers::Lock();
    const AddResult result = Base::ResourceBarriers::Add(id, barrier);

    if (id.GetType() != Barrier::Type::StateTransition)
        return result;

    switch (result)
    {
    case AddResult::Added:    AddNativeResourceBarrier(id, barrier.GetStateChange()); break;
    case AddResult::Updated:  UpdateNativeResourceBarrier(id, barrier.GetStateChange()); break;
    case AddResult::Existing: break;
    default: META_UNEXPECTED_RETURN(result, result);
    }
    return result;
}

bool ResourceBarriers::Remove(const Barrier::Id& id)
{
    META_FUNCTION_TASK();
    const auto lock_guard = Base::ResourceBarriers::Lock();
    if (!Base::ResourceBarriers::Remove(id))
        return false;

    if (id.GetType() != Barrier::Type::StateTransition)
        return true;

    const D3D12_RESOURCE_BARRIER_TYPE native_barrier_type = GetNativeBarrierType(id.GetType());
    const ID3D12Resource* native_resource_ptr = dynamic_cast<const IResource&>(id.GetResource()).GetNativeResource();
    const auto native_resource_barrier_it = std::find_if(m_native_resource_barriers.begin(), m_native_resource_barriers.end(),
                                                         GetNativeResourceBarrierPredicate(native_barrier_type, native_resource_ptr));
    META_CHECK_TRUE_DESCR(native_resource_barrier_it != m_native_resource_barriers.end(), "can not find DX resource barrier to update");
    m_native_resource_barriers.erase(native_resource_barrier_it);

    static_cast<Data::IEmitter<IResourceCallback>&>(id.GetResource()).Disconnect(*this);
    return true;
}

void ResourceBarriers::OnResourceReleased(Rhi::IResource& resource)
{
    META_FUNCTION_TASK();
    RemoveStateTransition(resource);
}

void ResourceBarriers::AddNativeResourceBarrier(const Barrier::Id& id, const Barrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    static_cast<Data::IEmitter<IResourceCallback>&>(id.GetResource()).Connect(*this);
    m_native_resource_barriers.emplace_back(GetNativeResourceBarrier(id, state_change));
}

void ResourceBarriers::UpdateNativeResourceBarrier(const Barrier::Id& id, const Barrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    const D3D12_RESOURCE_BARRIER_TYPE native_barrier_type = GetNativeBarrierType(id.GetType());
    const ID3D12Resource* native_resource_ptr = dynamic_cast<const IResource&>(id.GetResource()).GetNativeResource();
    const auto native_resource_barrier_it = std::find_if(m_native_resource_barriers.begin(), m_native_resource_barriers.end(),
                                                         GetNativeResourceBarrierPredicate(native_barrier_type, native_resource_ptr));
    META_CHECK_TRUE_DESCR(native_resource_barrier_it != m_native_resource_barriers.end(), "can not find DX resource barrier to update");

    switch (native_barrier_type) // NOSONAR - do not replace switch with if
    {
    case D3D12_RESOURCE_BARRIER_TYPE_TRANSITION:
        native_resource_barrier_it->Transition.StateBefore = IResource::GetNativeResourceState(state_change.GetStateBefore());
        native_resource_barrier_it->Transition.StateAfter  = IResource::GetNativeResourceState(state_change.GetStateAfter());
        break;

    default:
        META_UNEXPECTED(native_barrier_type);
    }
}

} // namespace Methane::Graphics