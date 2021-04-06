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

FILE: Methane/Graphics/DirectX12/ResourceDX.cpp
DirectX 12 implementation of the resource interface.

******************************************************************************/

#include "ResourceDX.h"
#include "DescriptorHeapDX.h"
#include "RenderContextDX.h"
#include "DeviceDX.h"

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <nowide/convert.hpp>
#include <algorithm>

namespace Methane::Graphics
{

static D3D12_RESOURCE_BARRIER_TYPE GetNativeBarrierType(Resource::Barrier::Type barrier_type)
{
    META_FUNCTION_TASK();
    switch (barrier_type) // NOSONAR
    {
    case Resource::Barrier::Type::Transition: return D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    default: META_UNEXPECTED_ARG_RETURN(barrier_type, D3D12_RESOURCE_BARRIER_TYPE_TRANSITION);
    }
}

static std::function<bool(const D3D12_RESOURCE_BARRIER&)> GetNativeResourceBarrierPredicate(D3D12_RESOURCE_BARRIER_TYPE native_barrier_type, const ID3D12Resource* native_resource_ptr)
{
    META_FUNCTION_TASK();
    switch (native_barrier_type)
    {
    case D3D12_RESOURCE_BARRIER_TYPE_TRANSITION:
        return [native_resource_ptr](const D3D12_RESOURCE_BARRIER& native_resource_barrier) -> bool
        { return native_resource_barrier.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION && native_resource_barrier.Transition.pResource == native_resource_ptr; };

    case D3D12_RESOURCE_BARRIER_TYPE_UAV:
        return [native_resource_ptr](const D3D12_RESOURCE_BARRIER& native_resource_barrier) -> bool
        { return native_resource_barrier.Type == D3D12_RESOURCE_BARRIER_TYPE_UAV && native_resource_barrier.UAV.pResource == native_resource_ptr; };

    case D3D12_RESOURCE_BARRIER_TYPE_ALIASING:
        return [native_resource_ptr](const D3D12_RESOURCE_BARRIER& native_resource_barrier) -> bool
        { return native_resource_barrier.Type == D3D12_RESOURCE_BARRIER_TYPE_ALIASING && native_resource_barrier.Aliasing.pResourceBefore == native_resource_ptr; };

    default: META_UNEXPECTED_ARG_RETURN(native_barrier_type, nullptr);
    }
}

Ptr<Resource::Barriers> Resource::Barriers::Create(const Set& barriers)
{
    META_FUNCTION_TASK();
    return std::make_shared<IResourceDX::BarriersDX>(barriers);
}

IResourceDX::BarriersDX::BarriersDX(const Set& barriers)
    : Barriers(barriers)
{
    META_FUNCTION_TASK();
    std::transform(barriers.begin(), barriers.end(), std::back_inserter(m_native_resource_barriers),
        [this](const Barrier& resource_barrier)
        {
            const_cast<Resource&>(resource_barrier.GetId().GetResource()).Connect(*this);
            return GetNativeResourceBarrier(resource_barrier);
        }
    );
}

Resource::Barriers::AddResult IResourceDX::BarriersDX::AddStateChange(const Barrier::Id& id, const Barrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    const auto lock_guard = Resource::Barriers::Lock();

    const AddResult result = Resource::Barriers::AddStateChange(id, state_change);
    switch (result)
    {
    case AddResult::Added:
        const_cast<Resource&>(id.GetResource()).Connect(*this);
        m_native_resource_barriers.emplace_back(GetNativeResourceBarrier(id, state_change));
        break;

    case AddResult::Updated:
        UpdateNativeResourceBarrier(id, state_change);
        break;

    case AddResult::Existing: break;
    default: META_UNEXPECTED_ARG_RETURN(result, result);
    }
    return result;
}

bool IResourceDX::BarriersDX::Remove(const Barrier::Id& id)
{
    META_FUNCTION_TASK();
    const auto lock_guard = Resource::Barriers::Lock();
    if (!Barriers::Remove(id))
        return false;

    const D3D12_RESOURCE_BARRIER_TYPE native_barrier_type = GetNativeBarrierType(id.GetType());
    const ID3D12Resource* native_resource_ptr = dynamic_cast<const IResourceDX&>(id.GetResource()).GetNativeResource();
    const auto native_resource_barrier_it = std::find_if(m_native_resource_barriers.begin(), m_native_resource_barriers.end(), GetNativeResourceBarrierPredicate(native_barrier_type, native_resource_ptr));
    META_CHECK_ARG_TRUE_DESCR(native_resource_barrier_it != m_native_resource_barriers.end(), "can not find DX resource barrier to update");
    m_native_resource_barriers.erase(native_resource_barrier_it);
    const_cast<Resource&>(id.GetResource()).Disconnect(*this);

    return true;
}

void IResourceDX::BarriersDX::OnResourceReleased(const Resource& resource)
{
    META_FUNCTION_TASK();
    RemoveTransition(resource);
}

void IResourceDX::BarriersDX::UpdateNativeResourceBarrier(const Barrier::Id& id, const Barrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    const D3D12_RESOURCE_BARRIER_TYPE native_barrier_type = GetNativeBarrierType(id.GetType());
    const ID3D12Resource* native_resource_ptr = dynamic_cast<const IResourceDX&>(id.GetResource()).GetNativeResource();
    const auto native_resource_barrier_it = std::find_if(m_native_resource_barriers.begin(), m_native_resource_barriers.end(), GetNativeResourceBarrierPredicate(native_barrier_type, native_resource_ptr));
    META_CHECK_ARG_TRUE_DESCR(native_resource_barrier_it != m_native_resource_barriers.end(), "can not find DX resource barrier to update");

    switch (native_barrier_type)
    {
    case D3D12_RESOURCE_BARRIER_TYPE_TRANSITION:
        native_resource_barrier_it->Transition.StateBefore = GetNativeResourceState(state_change.GetStateBefore());
        native_resource_barrier_it->Transition.StateAfter  = GetNativeResourceState(state_change.GetStateAfter());
        break;

    default: META_UNEXPECTED_ARG_RETURN(native_barrier_type, nullptr);
    }
}

IResourceDX::LocationDX::LocationDX(const Location& location)
    : Location(location)
    , m_resource_dx(dynamic_cast<IResourceDX&>(GetResource()))
{
    META_FUNCTION_TASK();
}

D3D12_RESOURCE_STATES IResourceDX::GetNativeResourceState(State resource_state)
{
    META_FUNCTION_TASK();
    switch (resource_state)
    {
    case State::Common:                    return D3D12_RESOURCE_STATE_COMMON;
    case State::VertexAndConstantBuffer:   return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    case State::IndexBuffer:               return D3D12_RESOURCE_STATE_INDEX_BUFFER;
    case State::RenderTarget:              return D3D12_RESOURCE_STATE_RENDER_TARGET;
    case State::UnorderedAccess:           return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    case State::DepthWrite:                return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    case State::DepthRead:                 return D3D12_RESOURCE_STATE_DEPTH_READ;
    case State::NonPixelShaderResource:    return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    case State::PixelShaderResource:       return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    case State::StreamOut:                 return D3D12_RESOURCE_STATE_STREAM_OUT;
    case State::IndirectArgument:          return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
    case State::CopyDest:                  return D3D12_RESOURCE_STATE_COPY_DEST;
    case State::CopySource:                return D3D12_RESOURCE_STATE_COPY_SOURCE;
    case State::ResolveDest:               return D3D12_RESOURCE_STATE_RESOLVE_DEST;
    case State::ResolveSource:             return D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
    case State::GenericRead:               return D3D12_RESOURCE_STATE_GENERIC_READ;
    case State::Present:                   return D3D12_RESOURCE_STATE_PRESENT;
    case State::Predication:               return D3D12_RESOURCE_STATE_PREDICATION;
    default:                               META_UNEXPECTED_ARG_RETURN(resource_state, D3D12_RESOURCE_STATE_COMMON);
    }
}

D3D12_RESOURCE_BARRIER IResourceDX::GetNativeResourceBarrier(const Barrier::Id& id, const Barrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    switch (id.GetType()) // NOSONAR
    {
    case Barrier::Type::Transition:
        return CD3DX12_RESOURCE_BARRIER::Transition(
            dynamic_cast<const IResourceDX&>(id.GetResource()).GetNativeResource(),
            GetNativeResourceState(state_change.GetStateBefore()),
            GetNativeResourceState(state_change.GetStateAfter())
        );

    default:
        META_UNEXPECTED_ARG_RETURN(id.GetType(), D3D12_RESOURCE_BARRIER());
    }
}

} // namespace Methane::Graphics
