/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
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
#include <Methane/Graphics/Windows/Primitives.h>

#include <nowide/convert.hpp>
#include <cassert>

namespace Methane::Graphics
{

Ptr<ResourceBase::Barriers> ResourceBase::Barriers::Create(const Set& barriers)
{
    META_FUNCTION_TASK();
    return std::make_shared<ResourceDX::BarriersDX>(barriers);
}

ResourceDX::BarriersDX::BarriersDX(const Set& barriers)
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

bool ResourceDX::BarriersDX::Add(const Barrier::Id& id, const Barrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    bool changed = ResourceBase::Barriers::Add(id, state_change);
    if (changed)
    {
        m_native_resource_barriers.emplace_back(GetNativeResourceBarrier(id, state_change));
    }
    return changed;
}


ResourceDX::LocationDX::LocationDX(const Location& location)
    : Location(location)
    , m_resource_dx(dynamic_cast<ResourceDX&>(GetResource()))
{
    META_FUNCTION_TASK();
}

ResourceDX::LocationDX::LocationDX(const LocationDX& location)
    : Location(location)
    , m_resource_dx(dynamic_cast<ResourceDX&>(GetResource()))
{
    META_FUNCTION_TASK();
}

ResourceDX::LocationDX& ResourceDX::LocationDX::operator=(const LocationDX& other)
{
    Location::operator=(other);
    m_resource_dx = dynamic_cast<ResourceDX&>(GetResource());
    return *this;
}


ResourceDX::ResourceDX(Type type, Usage::Mask usage_mask, ContextBase& context, const DescriptorByUsage& descriptor_by_usage)
    : ResourceBase(type, usage_mask, context, descriptor_by_usage)
{
    META_FUNCTION_TASK();
}

ResourceDX::~ResourceDX()
{
    META_FUNCTION_TASK();
    GetContextBase().GetResourceManager().GetReleasePool().AddResource(*this);
}

void ResourceDX::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    ResourceBase::SetName(name);
    
    // NOTE: there is no native resource for some resources like DX SamplerBase
    if (m_cp_resource)
    {
        m_cp_resource->SetName(nowide::widen(name).c_str());
    }
}

ID3D12Resource& ResourceDX::GetNativeResourceRef() const
{
    META_FUNCTION_TASK();
    assert(!!m_cp_resource);
    return *m_cp_resource.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE ResourceDX::GetNativeCpuDescriptorHandle(const Descriptor& desc) const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<const DescriptorHeapDX&>(desc.heap).GetNativeCpuDescriptorHandle(desc.index);
}

D3D12_GPU_DESCRIPTOR_HANDLE ResourceDX::GetNativeGpuDescriptorHandle(const Descriptor& desc) const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<const DescriptorHeapDX&>(desc.heap).GetNativeGpuDescriptorHandle(desc.index);
}

D3D12_RESOURCE_STATES ResourceDX::GetNativeResourceState(State resource_state) noexcept
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
    default:                               assert(0);
    }
    return D3D12_RESOURCE_STATE_COMMON;
}

D3D12_RESOURCE_BARRIER ResourceDX::GetNativeResourceBarrier(const Barrier::Id& id, const Barrier::StateChange& state_change) noexcept
{
    META_FUNCTION_TASK();
    switch (id.type)
    {
    case Barrier::Type::Transition:
        return CD3DX12_RESOURCE_BARRIER::Transition(
            dynamic_cast<const ResourceDX&>(id.resource).GetNativeResource(),
            GetNativeResourceState(state_change.before),
            GetNativeResourceState(state_change.after)
        );
    default:
        assert(0);
    }
    return D3D12_RESOURCE_BARRIER();
}

IContextDX& ResourceDX::GetContextDX() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<IContextDX&>(GetContextBase());
}

void ResourceDX::InitializeCommittedResource(const D3D12_RESOURCE_DESC& resource_desc, D3D12_HEAP_TYPE heap_type, 
                                             D3D12_RESOURCE_STATES resource_state, const D3D12_CLEAR_VALUE* p_clear_value)
{
    META_FUNCTION_TASK();
    assert(!m_cp_resource);

    const CD3DX12_HEAP_PROPERTIES heap_properties(heap_type);
    const wrl::ComPtr<ID3D12Device>& cp_native_device = GetContextDX().GetDeviceDX().GetNativeDevice();
    ThrowIfFailed(
        cp_native_device->CreateCommittedResource(
            &heap_properties,
            D3D12_HEAP_FLAG_NONE,
            &resource_desc,
            resource_state,
            p_clear_value,
            IID_PPV_ARGS(&m_cp_resource)
        ),
        cp_native_device.Get()
    );
}

void ResourceDX::InitializeFrameBufferResource(uint32_t frame_buffer_index)
{
    META_FUNCTION_TASK();
    assert(!m_cp_resource);

    ThrowIfFailed(
        static_cast<RenderContextDX&>(GetContextDX()).GetNativeSwapChain()->GetBuffer(
            frame_buffer_index,
            IID_PPV_ARGS(&m_cp_resource)
        ),
        GetContextDX().GetDeviceDX().GetNativeDevice().Get()
    );
}

} // namespace Methane::Graphics
