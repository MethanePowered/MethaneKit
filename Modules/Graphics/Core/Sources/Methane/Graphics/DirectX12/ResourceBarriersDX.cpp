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

FILE: Methane/Graphics/DirectX12/ResourceBarriersDX.cpp
DirectX 12 specialization of the resource barriers.

******************************************************************************/

#include "ResourceBarriersDX.h"
#include "ResourceDX.h"

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics
{

D3D12_RESOURCE_STATES ResourceBarriersDX::GetNativeResourceState(ResourceState resource_state)
{
    META_FUNCTION_TASK();
    switch (resource_state)
    {
    case ResourceState::Undefined:        return D3D12_RESOURCE_STATE_COMMON;
    case ResourceState::Common:           return D3D12_RESOURCE_STATE_COMMON;
    case ResourceState::VertexBuffer:     return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    case ResourceState::ConstantBuffer:   return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    case ResourceState::IndexBuffer:      return D3D12_RESOURCE_STATE_INDEX_BUFFER;
    case ResourceState::RenderTarget:     return D3D12_RESOURCE_STATE_RENDER_TARGET;
    case ResourceState::InputAttachment:  return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    case ResourceState::UnorderedAccess:  return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    case ResourceState::DepthWrite:       return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    case ResourceState::DepthRead:        return D3D12_RESOURCE_STATE_DEPTH_READ;
    case ResourceState::ShaderResource:   return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
                                                 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    case ResourceState::StreamOut:        return D3D12_RESOURCE_STATE_STREAM_OUT;
    case ResourceState::IndirectArgument: return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
    case ResourceState::CopyDest:         return D3D12_RESOURCE_STATE_COPY_DEST;
    case ResourceState::CopySource:       return D3D12_RESOURCE_STATE_COPY_SOURCE;
    case ResourceState::ResolveDest:      return D3D12_RESOURCE_STATE_RESOLVE_DEST;
    case ResourceState::ResolveSource:    return D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
    case ResourceState::GenericRead:      return D3D12_RESOURCE_STATE_GENERIC_READ;
    case ResourceState::Present:          return D3D12_RESOURCE_STATE_PRESENT;
    default: META_UNEXPECTED_ARG_RETURN(resource_state, D3D12_RESOURCE_STATE_COMMON);
    }
}

D3D12_RESOURCE_BARRIER ResourceBarriersDX::GetNativeResourceBarrier(const ResourceBarrier::Id& id, const ResourceBarrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    switch (id.GetType()) // NOSONAR
    {
    case ResourceBarrier::Type::StateTransition:
        return CD3DX12_RESOURCE_BARRIER::Transition(
            dynamic_cast<const IResourceDX&>(id.GetResource()).GetNativeResource(),
            GetNativeResourceState(state_change.GetStateBefore()),
            GetNativeResourceState(state_change.GetStateAfter())
        );

    default:
        META_UNEXPECTED_ARG_RETURN(id.GetType(), D3D12_RESOURCE_BARRIER());
    }
}

[[nodiscard]]
static D3D12_RESOURCE_BARRIER_TYPE GetNativeBarrierType(ResourceBarrier::Type barrier_type)
{
    META_FUNCTION_TASK();
    switch (barrier_type) // NOSONAR
    {
    case ResourceBarrier::Type::StateTransition: return D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    default: META_UNEXPECTED_ARG_RETURN(barrier_type, D3D12_RESOURCE_BARRIER_TYPE_TRANSITION);
    }
}

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
        META_UNEXPECTED_ARG_RETURN(native_barrier_type, nullptr);
    }
}

Ptr<ResourceBarriers> ResourceBarriers::Create(const Set& barriers)
{
    META_FUNCTION_TASK();
    return std::make_shared<ResourceBarriersDX>(barriers);
}

ResourceBarriersDX::ResourceBarriersDX(const Set& barriers)
    : ResourceBarriers(barriers)
{
    META_FUNCTION_TASK();
    for(const ResourceBarrier barrier : barriers)
    {
        AddNativeResourceBarrier(barrier.GetId(), barrier.GetStateChange());
    }
}

ResourceBarriers::AddResult ResourceBarriersDX::Add(const ResourceBarrier::Id& id, const ResourceBarrier& barrier)
{
    META_FUNCTION_TASK();
    const auto lock_guard  = ResourceBarriers::Lock();
    const AddResult result = ResourceBarriers::Add(id, barrier);

    if (id.GetType() != ResourceBarrier::Type::StateTransition)
        return result;

    switch (result)
    {
    case AddResult::Added:    AddNativeResourceBarrier(id, barrier.GetStateChange()); break;
    case AddResult::Updated:  UpdateNativeResourceBarrier(id, barrier.GetStateChange()); break;
    case AddResult::Existing: break;
    default: META_UNEXPECTED_ARG_RETURN(result, result);
    }
    return result;
}

bool ResourceBarriersDX::Remove(const ResourceBarrier::Id& id)
{
    META_FUNCTION_TASK();
    const auto lock_guard = ResourceBarriers::Lock();
    if (!ResourceBarriers::Remove(id))
        return false;

    if (id.GetType() != ResourceBarrier::Type::StateTransition)
        return true;

    const D3D12_RESOURCE_BARRIER_TYPE native_barrier_type = GetNativeBarrierType(id.GetType());
    const ID3D12Resource* native_resource_ptr = dynamic_cast<const IResourceDX&>(id.GetResource()).GetNativeResource();
    const auto native_resource_barrier_it = std::find_if(m_native_resource_barriers.begin(), m_native_resource_barriers.end(),
                                                         GetNativeResourceBarrierPredicate(native_barrier_type, native_resource_ptr));
    META_CHECK_ARG_TRUE_DESCR(native_resource_barrier_it != m_native_resource_barriers.end(), "can not find DX resource barrier to update");
    m_native_resource_barriers.erase(native_resource_barrier_it);

    static_cast<Data::IEmitter<IResourceCallback>&>(id.GetResource()).Disconnect(*this);
    return true;
}

void ResourceBarriersDX::OnResourceReleased(Resource& resource)
{
    META_FUNCTION_TASK();
    RemoveStateTransition(resource);
}

void ResourceBarriersDX::AddNativeResourceBarrier(const ResourceBarrier::Id& id, const ResourceBarrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    static_cast<Data::IEmitter<IResourceCallback>&>(id.GetResource()).Connect(*this);
    m_native_resource_barriers.emplace_back(GetNativeResourceBarrier(id, state_change));
}

void ResourceBarriersDX::UpdateNativeResourceBarrier(const ResourceBarrier::Id& id, const ResourceBarrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    const D3D12_RESOURCE_BARRIER_TYPE native_barrier_type = GetNativeBarrierType(id.GetType());
    const ID3D12Resource* native_resource_ptr = dynamic_cast<const IResourceDX&>(id.GetResource()).GetNativeResource();
    const auto native_resource_barrier_it = std::find_if(m_native_resource_barriers.begin(), m_native_resource_barriers.end(),
                                                         GetNativeResourceBarrierPredicate(native_barrier_type, native_resource_ptr));
    META_CHECK_ARG_TRUE_DESCR(native_resource_barrier_it != m_native_resource_barriers.end(), "can not find DX resource barrier to update");

    switch (native_barrier_type) // NOSONAR - do not replace switch with if
    {
    case D3D12_RESOURCE_BARRIER_TYPE_TRANSITION:
        native_resource_barrier_it->Transition.StateBefore = GetNativeResourceState(state_change.GetStateBefore());
        native_resource_barrier_it->Transition.StateAfter  = GetNativeResourceState(state_change.GetStateAfter());
        break;

    default:
        META_UNEXPECTED_ARG(native_barrier_type);
    }
}

} // namespace Methane::Graphics