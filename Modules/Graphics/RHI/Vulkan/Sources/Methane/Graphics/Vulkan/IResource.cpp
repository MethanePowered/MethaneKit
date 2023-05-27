/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/IResource.cpp
Vulkan implementation of the resource objects.

******************************************************************************/

#include <Methane/Graphics/Vulkan/IResource.h>
#include <Methane/Graphics/Vulkan/Types.h>

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Vulkan
{

vk::AccessFlags IResource::GetNativeAccessFlagsByResourceState(Rhi::ResourceState resource_state)
{
    META_FUNCTION_TASK();
    switch (resource_state)
    {
    case Rhi::ResourceState::Undefined:        return vk::AccessFlagBits::eNoneKHR;
    case Rhi::ResourceState::Common:           return vk::AccessFlagBits::eNoneKHR;
    case Rhi::ResourceState::VertexBuffer:     return vk::AccessFlagBits::eVertexAttributeRead;
    case Rhi::ResourceState::ConstantBuffer:   return vk::AccessFlagBits::eUniformRead;
    case Rhi::ResourceState::IndexBuffer:      return vk::AccessFlagBits::eIndexRead;
    case Rhi::ResourceState::RenderTarget:     return vk::AccessFlagBits::eColorAttachmentRead |
                                                      vk::AccessFlagBits::eColorAttachmentWrite;
    case Rhi::ResourceState::UnorderedAccess:  return vk::AccessFlagBits::eShaderRead |
                                                      vk::AccessFlagBits::eShaderWrite;
    case Rhi::ResourceState::DepthWrite:       return vk::AccessFlagBits::eDepthStencilAttachmentWrite |
                                                      vk::AccessFlagBits::eDepthStencilAttachmentRead;
    case Rhi::ResourceState::DepthRead:        return vk::AccessFlagBits::eDepthStencilAttachmentRead;
    case Rhi::ResourceState::ShaderResource:   return vk::AccessFlagBits::eShaderRead;
    case Rhi::ResourceState::IndirectArgument: return vk::AccessFlagBits::eIndirectCommandRead;
    case Rhi::ResourceState::CopyDest:         return vk::AccessFlagBits::eTransferWrite;
    case Rhi::ResourceState::CopySource:       return vk::AccessFlagBits::eTransferRead;
    case Rhi::ResourceState::ResolveDest:      return vk::AccessFlagBits::eTransferWrite;
    case Rhi::ResourceState::ResolveSource:    return vk::AccessFlagBits::eTransferRead;
    case Rhi::ResourceState::Present:          return vk::AccessFlagBits::eNoneKHR;
    case Rhi::ResourceState::GenericRead:      return vk::AccessFlagBits::eVertexAttributeRead |
                                                      vk::AccessFlagBits::eUniformRead |
                                                      vk::AccessFlagBits::eIndexRead |
                                                      vk::AccessFlagBits::eShaderRead |
                                                      vk::AccessFlagBits::eIndirectCommandRead |
                                                      vk::AccessFlagBits::eTransferRead;
    default: META_UNEXPECTED_ARG_DESCR_RETURN(resource_state, vk::AccessFlagBits::eNoneKHR, "unexpected resource state");
    }
}

vk::ImageLayout IResource::GetNativeImageLayoutByResourceState(Rhi::ResourceState resource_state)
{
    META_FUNCTION_TASK();
    switch (resource_state)
    {
    case Rhi::ResourceState::Undefined:       return vk::ImageLayout::eUndefined;
    case Rhi::ResourceState::Common:          return vk::ImageLayout::eGeneral;
    case Rhi::ResourceState::RenderTarget:    return vk::ImageLayout::eColorAttachmentOptimal;
    case Rhi::ResourceState::InputAttachment: return vk::ImageLayout::eShaderReadOnlyOptimal;
    case Rhi::ResourceState::UnorderedAccess: return vk::ImageLayout::eGeneral;
    case Rhi::ResourceState::DepthWrite:      return vk::ImageLayout::eDepthStencilAttachmentOptimal;
    case Rhi::ResourceState::DepthRead:       return vk::ImageLayout::eDepthStencilReadOnlyOptimal;
    case Rhi::ResourceState::ShaderResource:  return vk::ImageLayout::eShaderReadOnlyOptimal;
    case Rhi::ResourceState::CopyDest:        return vk::ImageLayout::eTransferDstOptimal;
    case Rhi::ResourceState::CopySource:      return vk::ImageLayout::eTransferSrcOptimal;
    case Rhi::ResourceState::ResolveDest:     return vk::ImageLayout::eTransferDstOptimal;
    case Rhi::ResourceState::ResolveSource:   return vk::ImageLayout::eTransferSrcOptimal;
    case Rhi::ResourceState::Present:         return vk::ImageLayout::ePresentSrcKHR;
    default: META_UNEXPECTED_ARG_DESCR_RETURN(resource_state, vk::ImageLayout::eUndefined, "unexpected resource state");
    }
}

vk::PipelineStageFlags IResource::GetNativePipelineStageFlagsByResourceState(Rhi::ResourceState resource_state)
{
    META_FUNCTION_TASK();
    switch (resource_state)
    {
    case Rhi::ResourceState::Undefined:
        return vk::PipelineStageFlagBits::eTopOfPipe;
    case Rhi::ResourceState::Common:
        return vk::PipelineStageFlagBits::eAllCommands;
    case Rhi::ResourceState::Present:
        return vk::PipelineStageFlagBits::eBottomOfPipe;
    case Rhi::ResourceState::RenderTarget:
        return vk::PipelineStageFlagBits::eColorAttachmentOutput;
    case Rhi::ResourceState::InputAttachment:
        return vk::PipelineStageFlagBits::eFragmentShader;
    case Rhi::ResourceState::IndirectArgument:
        return vk::PipelineStageFlagBits::eDrawIndirect;
    case Rhi::ResourceState::VertexBuffer:
    case Rhi::ResourceState::IndexBuffer:
        return vk::PipelineStageFlagBits::eVertexInput;
    case Rhi::ResourceState::GenericRead:
    case Rhi::ResourceState::ConstantBuffer:
    case Rhi::ResourceState::UnorderedAccess:
    case Rhi::ResourceState::ShaderResource:
        return vk::PipelineStageFlagBits::eVertexShader | // All possible shader stages
               vk::PipelineStageFlagBits::eFragmentShader |
               vk::PipelineStageFlagBits::eComputeShader;
    case Rhi::ResourceState::CopyDest:
    case Rhi::ResourceState::CopySource:
    case Rhi::ResourceState::ResolveDest:
    case Rhi::ResourceState::ResolveSource:
        return vk::PipelineStageFlagBits::eTransfer;
    case Rhi::ResourceState::DepthWrite:
    case Rhi::ResourceState::DepthRead:
        return vk::PipelineStageFlagBits::eEarlyFragmentTests |
               vk::PipelineStageFlagBits::eLateFragmentTests;
    case Rhi::ResourceState::StreamOut:
        return {};
    default:
        META_UNEXPECTED_ARG_DESCR_RETURN(resource_state, vk::ImageLayout::eUndefined, "unexpected resource state");
    }
}

} // using namespace Methane::Graphics::Vulkan
