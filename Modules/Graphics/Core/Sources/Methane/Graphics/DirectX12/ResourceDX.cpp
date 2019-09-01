/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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
#include "ContextDX.h"
#include "DeviceDX.h"

#include <Methane/Graphics/Instrumentation.h>
#include <Methane/Graphics/Windows/Helpers.h>

#include <nowide/convert.hpp>
#include <algorithm>
#include <iterator>
#include <cassert>

namespace Methane
{
namespace Graphics
{

ResourceBase::ReleasePool::Ptr ResourceBase::ReleasePool::Create()
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ResourceDX::ReleasePoolDX>();
}

void ResourceDX::ReleasePoolDX::AddResource(ResourceBase& resource)
{
    ITT_FUNCTION_TASK();
    ResourceDX& resource_dx = static_cast<ResourceDX&>(resource);
    m_resources.push_back(resource_dx.GetNativeResource());
}

void ResourceDX::ReleasePoolDX::ReleaseResources()
{
    ITT_FUNCTION_TASK();
    m_resources.clear();
}

ResourceDX::ResourceDX(Type type, Usage::Mask usage_mask, ContextBase& context, const DescriptorByUsage& descriptor_by_usage)
    : ResourceBase(type, usage_mask, context, descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
}

ResourceDX::~ResourceDX()
{
    ITT_FUNCTION_TASK();
    m_context.GetResourceManager().GetReleasePool().AddResource(*this);
}

void ResourceDX::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();

    ResourceBase::SetName(name);
    
    // NOTE: there is no native resource for some resources like DX SamplerBase
    if (m_cp_resource)
    {
        m_cp_resource->SetName(nowide::widen(name).c_str());
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE ResourceDX::GetNativeCPUDescriptorHandle(const Descriptor& desc) const noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<const DescriptorHeapDX&>(desc.heap).GetNativeCPUDescriptorHandle(desc.index);
}

D3D12_GPU_DESCRIPTOR_HANDLE ResourceDX::GetNativeGPUDescriptorHandle(const Descriptor& desc) const noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<const DescriptorHeapDX&>(desc.heap).GetNativeGPUDescriptorHandle(desc.index);
}

D3D12_RESOURCE_STATES ResourceDX::GetNativeResourceState(State resource_state) noexcept
{
    ITT_FUNCTION_TASK();
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

D3D12_RESOURCE_BARRIER ResourceDX::GetNativeResourceBarrier(const Barrier& resource_barrier) noexcept
{
    ITT_FUNCTION_TASK();
    switch (resource_barrier.type)
    {
    case Barrier::Type::Transition:
        return CD3DX12_RESOURCE_BARRIER::Transition(
            dynamic_cast<const ResourceDX&>(resource_barrier.resource).GetNativeResource(),
            GetNativeResourceState(resource_barrier.state_before),
            GetNativeResourceState(resource_barrier.state_after)
        );
    default:
        assert(0);
    }
    return D3D12_RESOURCE_BARRIER();
}

ContextDX& ResourceDX::GetContextDX() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<class ContextDX&>(m_context);
}

void ResourceDX::InitializeCommittedResource(const D3D12_RESOURCE_DESC& resource_desc, D3D12_HEAP_TYPE heap_type, 
                                             D3D12_RESOURCE_STATES resource_state, const D3D12_CLEAR_VALUE* p_clear_value)
{
    ITT_FUNCTION_TASK();

    assert(!m_cp_resource);
    ThrowIfFailed(
        GetContextDX().GetDeviceDX().GetNativeDevice()->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(heap_type),
            D3D12_HEAP_FLAG_NONE,
            &resource_desc,
            resource_state,
            p_clear_value,
            IID_PPV_ARGS(&m_cp_resource)
        )
    );
}

void ResourceDX::InitializeFrameBufferResource(uint32_t frame_buffer_index)
{
    ITT_FUNCTION_TASK();

    assert(!m_cp_resource);
    ThrowIfFailed(
        GetContextDX().GetNativeSwapChain()->GetBuffer(
            frame_buffer_index,
            IID_PPV_ARGS(&m_cp_resource)
        )
    );
}

} // namespace Graphics
} // namespace Methane
