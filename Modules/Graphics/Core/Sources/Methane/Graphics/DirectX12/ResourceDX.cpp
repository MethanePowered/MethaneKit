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

namespace Methane::Graphics
{

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
        [](const Barrier& resource_barrier)
        {
            return GetNativeResourceBarrier(resource_barrier);
        }
    );
}

bool IResourceDX::BarriersDX::AddStateChange(const Barrier::Id& id, const Barrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    bool changed = Resource::Barriers::AddStateChange(id, state_change);
    if (changed)
    {
        m_native_resource_barriers.emplace_back(GetNativeResourceBarrier(id, state_change));
    }
    return changed;
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
