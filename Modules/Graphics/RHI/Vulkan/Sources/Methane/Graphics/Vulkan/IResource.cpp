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
    using enum Rhi::ResourceState;
    using enum vk::AccessFlagBits;
    case Undefined:        return eNoneKHR;
    case Common:           return eNoneKHR;
    case VertexBuffer:     return eVertexAttributeRead;
    case ConstantBuffer:   return eUniformRead;
    case IndexBuffer:      return eIndexRead;
    case RenderTarget:     return eColorAttachmentRead |
                                  eColorAttachmentWrite;
    case UnorderedAccess:  return eShaderRead |
                                  eShaderWrite;
    case DepthWrite:       return eDepthStencilAttachmentWrite |
                                  eDepthStencilAttachmentRead;
    case DepthRead:        return eDepthStencilAttachmentRead;
    case ShaderResource:   return eShaderRead;
    case IndirectArgument: return eIndirectCommandRead;
    case CopyDest:         return eTransferWrite;
    case CopySource:       return eTransferRead;
    case ResolveDest:      return eTransferWrite;
    case ResolveSource:    return eTransferRead;
    case Present:          return eNoneKHR;
    case GenericRead:      return eVertexAttributeRead |
                                  eUniformRead |
                                  eIndexRead |
                                  eShaderRead |
                                  eIndirectCommandRead |
                                  eTransferRead;
    default: META_UNEXPECTED_RETURN_DESCR(resource_state, eNoneKHR, "unexpected resource state");
    }
}

vk::ImageLayout IResource::GetNativeImageLayoutByResourceState(Rhi::ResourceState resource_state)
{
    META_FUNCTION_TASK();
    switch (resource_state)
    {
    using enum Rhi::ResourceState;
    using enum vk::ImageLayout;
    case Undefined:       return eUndefined;
    case Common:          return eGeneral;
    case RenderTarget:    return eColorAttachmentOptimal;
    case InputAttachment: return eShaderReadOnlyOptimal;
    case UnorderedAccess: return eGeneral;
    case DepthWrite:      return eDepthStencilAttachmentOptimal;
    case DepthRead:       return eDepthStencilReadOnlyOptimal;
    case ShaderResource:  return eShaderReadOnlyOptimal;
    case CopyDest:        return eTransferDstOptimal;
    case CopySource:      return eTransferSrcOptimal;
    case ResolveDest:     return eTransferDstOptimal;
    case ResolveSource:   return eTransferSrcOptimal;
    case Present:         return ePresentSrcKHR;
    default: META_UNEXPECTED_RETURN_DESCR(resource_state, eUndefined, "unexpected resource state");
    }
}

vk::PipelineStageFlags IResource::GetNativePipelineStageFlagsByResourceState(Rhi::ResourceState resource_state)
{
    META_FUNCTION_TASK();
    switch (resource_state)
    {
    using enum Rhi::ResourceState;
    using enum vk::PipelineStageFlagBits;
    case Undefined:        return eTopOfPipe;
    case Common:           return eAllCommands;
    case Present:          return eBottomOfPipe;
    case RenderTarget:     return eColorAttachmentOutput;
    case InputAttachment:  return eFragmentShader;
    case IndirectArgument: return eDrawIndirect;
    case VertexBuffer:
    case IndexBuffer:
        return eVertexInput;
    case GenericRead:
    case ConstantBuffer:
    case UnorderedAccess:
    case ShaderResource:
        return eVertexShader | // All possible shader stages
               eFragmentShader |
               eComputeShader;
    case CopyDest:
    case CopySource:
    case ResolveDest:
    case ResolveSource:
        return eTransfer;
    case DepthWrite:
    case DepthRead:
        return eEarlyFragmentTests |
               eLateFragmentTests;
    case StreamOut:
        return {};
    default:
        META_UNEXPECTED_RETURN_DESCR(resource_state, eUndefined, "unexpected resource state");
    }
}

} // using namespace Methane::Graphics::Vulkan
