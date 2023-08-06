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

FILE: Methane/Graphics/DirectX/IResource.cpp
DirectX 12 specialization of the resource interface.

******************************************************************************/

#include <Methane/Graphics/DirectX/IResource.h>
#include <Methane/Graphics/RHI/ITexture.h>
#include <Methane/Data/EnumMaskUtil.hpp>

template<>
struct fmt::formatter<Methane::Graphics::Rhi::ResourceUsage>
{
    template<typename FormatContext>
    [[nodiscard]] auto format(const Methane::Graphics::Rhi::ResourceUsage& rl, FormatContext& ctx) const
    { return format_to(ctx.out(), "{}", fmt::join(rl.GetBitNames(), "|")); }

    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const
    { return ctx.end(); }
};

namespace Methane::Graphics::DirectX
{

DescriptorHeap::Type IResource::GetDescriptorHeapTypeByUsage(const Rhi::IResource& resource, UsageMask resource_usage)
{
    META_FUNCTION_TASK();
    const Rhi::IResource::Type resource_type = resource.GetResourceType();
    if (resource_usage.HasAnyBits({ Usage::ShaderRead, Usage::ShaderWrite }))
    {
        return (resource_type == Type::Sampler)
               ? DescriptorHeap::Type::Samplers
               : DescriptorHeap::Type::ShaderResources;
    }
    else if (resource_usage.HasAnyBit(Usage::RenderTarget))
    {
        return (resource_type == Type::Texture &&
                dynamic_cast<const Rhi::ITexture&>(resource).GetSettings().type == Rhi::TextureType::DepthStencil)
               ? DescriptorHeap::Type::DepthStencil
               : DescriptorHeap::Type::RenderTargets;
    }
    else
    {
        META_UNEXPECTED_ARG_DESCR_RETURN(resource_usage.GetValue(), DescriptorHeap::Type::Undefined,
                                         "resource usage {} does not map to descriptor heap", Data::GetEnumMaskName(resource_usage));
    }
}

D3D12_RESOURCE_STATES IResource::GetNativeResourceState(State resource_state)
{
    META_FUNCTION_TASK();
    switch (resource_state)
    {
    case State::Undefined:        return D3D12_RESOURCE_STATE_COMMON;
    case State::Common:           return D3D12_RESOURCE_STATE_COMMON;
    case State::VertexBuffer:     return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    case State::ConstantBuffer:   return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    case State::IndexBuffer:      return D3D12_RESOURCE_STATE_INDEX_BUFFER;
    case State::RenderTarget:     return D3D12_RESOURCE_STATE_RENDER_TARGET;
    case State::InputAttachment:  return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    case State::UnorderedAccess:  return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    case State::DepthWrite:       return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    case State::DepthRead:        return D3D12_RESOURCE_STATE_DEPTH_READ;
    case State::ShaderResource:   return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
                                         D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    case State::StreamOut:        return D3D12_RESOURCE_STATE_STREAM_OUT;
    case State::IndirectArgument: return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
    case State::CopyDest:         return D3D12_RESOURCE_STATE_COPY_DEST;
    case State::CopySource:       return D3D12_RESOURCE_STATE_COPY_SOURCE;
    case State::ResolveDest:      return D3D12_RESOURCE_STATE_RESOLVE_DEST;
    case State::ResolveSource:    return D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
    case State::GenericRead:      return D3D12_RESOURCE_STATE_GENERIC_READ;
    case State::Present:          return D3D12_RESOURCE_STATE_PRESENT;
    default: META_UNEXPECTED_ARG_RETURN(resource_state, D3D12_RESOURCE_STATE_COMMON);
    }
}

} // namespace Methane::Graphics::DirectX
