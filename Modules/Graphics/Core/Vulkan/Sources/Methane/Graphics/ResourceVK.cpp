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

FILE: Methane/Graphics/Vulkan/ResourceVK.cpp
Vulkan implementation of the resource objects.

******************************************************************************/

#include <Methane/Graphics/ResourceVK.h>
#include <Methane/Graphics/BufferVK.h>
#include <Methane/Graphics/TextureVK.h>
#include <Methane/Graphics/SamplerVK.h>
#include <Methane/Graphics/TypesVK.h>
#include <Methane/Graphics/UtilsVK.hpp>

#include <Methane/Instrumentation.h>
#include <fmt/format.h>

namespace Methane::Graphics
{

ResourceViewVK::ResourceViewVK(const ResourceView& view_id, IResource::Usage usage)
    : ResourceView(view_id)
    , m_id(usage, GetSettings())
    , m_vulkan_resource_ref(dynamic_cast<IResourceVK&>(GetResource()))
    , m_view_desc_var_ptr(m_vulkan_resource_ref.get().InitializeNativeViewDescriptor(m_id))
{
    META_FUNCTION_TASK();
}

IResourceVK& ResourceViewVK::GetResourceVK() const
{
    META_FUNCTION_TASK();
    return m_vulkan_resource_ref.get();
}

const ResourceViewVK::BufferViewDescriptor* ResourceViewVK::GetBufferViewDescriptorPtr() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_view_desc_var_ptr);
    return std::get_if<BufferViewDescriptor>(m_view_desc_var_ptr.get());
}

const ResourceViewVK::BufferViewDescriptor& ResourceViewVK::GetBufferViewDescriptor() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_view_desc_var_ptr);
    return std::get<BufferViewDescriptor>(*m_view_desc_var_ptr);
}

const ResourceViewVK::ImageViewDescriptor* ResourceViewVK::GetImageViewDescriptorPtr() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_view_desc_var_ptr);
    return std::get_if<ImageViewDescriptor>(m_view_desc_var_ptr.get());
}

const ResourceViewVK::ImageViewDescriptor& ResourceViewVK::GetImageViewDescriptor() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_view_desc_var_ptr);
    return std::get<ImageViewDescriptor>(*m_view_desc_var_ptr);
}

const vk::DescriptorBufferInfo* ResourceViewVK::GetNativeDescriptorBufferInfoPtr() const
{
    META_FUNCTION_TASK();
    const BufferViewDescriptor* buffer_view_desc_ptr = GetBufferViewDescriptorPtr();
    return buffer_view_desc_ptr ? &buffer_view_desc_ptr->vk_desc : nullptr;
}

const vk::DescriptorImageInfo* ResourceViewVK::GetNativeDescriptorImageInfoPtr() const
{
    META_FUNCTION_TASK();
    const ImageViewDescriptor* image_view_desc_ptr = GetImageViewDescriptorPtr();
    return image_view_desc_ptr ? &image_view_desc_ptr->vk_desc : nullptr;
}

const vk::BufferView* ResourceViewVK::GetNativeBufferViewPtr() const
{
    META_FUNCTION_TASK();
    const BufferViewDescriptor* buffer_view_desc_ptr = GetBufferViewDescriptorPtr();
    return buffer_view_desc_ptr ? &buffer_view_desc_ptr->vk_view.get() : nullptr;
}

const vk::ImageView* ResourceViewVK::GetNativeImageViewPtr() const
{
    META_FUNCTION_TASK();
    const ImageViewDescriptor* image_view_desc_ptr = GetImageViewDescriptorPtr();
    return image_view_desc_ptr ? &image_view_desc_ptr->vk_view.get() : nullptr;
}

const vk::BufferView& ResourceViewVK::GetNativeBufferView() const
{
    META_FUNCTION_TASK();
    const BufferViewDescriptor& buffer_view_desc = GetBufferViewDescriptor();
    META_CHECK_ARG_NOT_NULL(buffer_view_desc.vk_view);
    return buffer_view_desc.vk_view.get();
}

const vk::ImageView& ResourceViewVK::GetNativeImageView() const
{
    META_FUNCTION_TASK();
    const ImageViewDescriptor& image_view_desc = GetImageViewDescriptor();
    META_CHECK_ARG_NOT_NULL(image_view_desc.vk_view);
    return image_view_desc.vk_view.get();
}

vk::AccessFlags IResourceVK::GetNativeAccessFlagsByResourceState(ResourceState resource_state)
{
    META_FUNCTION_TASK();
    switch (resource_state)
    {
    case ResourceState::Undefined:        return vk::AccessFlagBits::eNoneKHR;
    case ResourceState::Common:           return vk::AccessFlagBits::eNoneKHR;
    case ResourceState::VertexBuffer:     return vk::AccessFlagBits::eVertexAttributeRead;
    case ResourceState::ConstantBuffer:   return vk::AccessFlagBits::eUniformRead;
    case ResourceState::IndexBuffer:      return vk::AccessFlagBits::eIndexRead;
    case ResourceState::RenderTarget:     return vk::AccessFlagBits::eColorAttachmentRead |
                                                 vk::AccessFlagBits::eColorAttachmentWrite;
    case ResourceState::UnorderedAccess:  return vk::AccessFlagBits::eShaderRead |
                                                 vk::AccessFlagBits::eShaderWrite;
    case ResourceState::DepthWrite:       return vk::AccessFlagBits::eDepthStencilAttachmentWrite |
                                                 vk::AccessFlagBits::eDepthStencilAttachmentRead;
    case ResourceState::DepthRead:        return vk::AccessFlagBits::eDepthStencilAttachmentRead;
    case ResourceState::ShaderResource:   return vk::AccessFlagBits::eShaderRead;
    case ResourceState::IndirectArgument: return vk::AccessFlagBits::eIndirectCommandRead;
    case ResourceState::CopyDest:         return vk::AccessFlagBits::eTransferWrite;
    case ResourceState::CopySource:       return vk::AccessFlagBits::eTransferRead;
    case ResourceState::ResolveDest:      return vk::AccessFlagBits::eTransferWrite;
    case ResourceState::ResolveSource:    return vk::AccessFlagBits::eTransferRead;
    case ResourceState::Present:          return vk::AccessFlagBits::eNoneKHR;
    case ResourceState::GenericRead:      return vk::AccessFlagBits::eVertexAttributeRead |
                                                 vk::AccessFlagBits::eUniformRead |
                                                 vk::AccessFlagBits::eIndexRead |
                                                 vk::AccessFlagBits::eShaderRead |
                                                 vk::AccessFlagBits::eIndirectCommandRead |
                                                 vk::AccessFlagBits::eTransferRead;
    default: META_UNEXPECTED_ARG_DESCR_RETURN(resource_state, vk::AccessFlagBits::eNoneKHR, "unexpected resource state");
    }
}

vk::ImageLayout IResourceVK::GetNativeImageLayoutByResourceState(ResourceState resource_state)
{
    META_FUNCTION_TASK();
    switch (resource_state)
    {
    case ResourceState::Undefined:       return vk::ImageLayout::eUndefined;
    case ResourceState::Common:          return vk::ImageLayout::eGeneral;
    case ResourceState::RenderTarget:    return vk::ImageLayout::eColorAttachmentOptimal;
    case ResourceState::InputAttachment: return vk::ImageLayout::eShaderReadOnlyOptimal;
    case ResourceState::UnorderedAccess: return vk::ImageLayout::eGeneral;
    case ResourceState::DepthWrite:      return vk::ImageLayout::eDepthStencilAttachmentOptimal;
    case ResourceState::DepthRead:       return vk::ImageLayout::eDepthStencilReadOnlyOptimal;
    case ResourceState::ShaderResource:  return vk::ImageLayout::eShaderReadOnlyOptimal;
    case ResourceState::CopyDest:        return vk::ImageLayout::eTransferDstOptimal;
    case ResourceState::CopySource:      return vk::ImageLayout::eTransferSrcOptimal;
    case ResourceState::ResolveDest:     return vk::ImageLayout::eTransferDstOptimal;
    case ResourceState::ResolveSource:   return vk::ImageLayout::eTransferSrcOptimal;
    case ResourceState::Present:         return vk::ImageLayout::ePresentSrcKHR;
    default: META_UNEXPECTED_ARG_DESCR_RETURN(resource_state, vk::ImageLayout::eUndefined, "unexpected resource state");
    }
}

vk::PipelineStageFlags IResourceVK::GetNativePipelineStageFlagsByResourceState(ResourceState resource_state)
{
    META_FUNCTION_TASK();
    switch (resource_state)
    {
    case ResourceState::Undefined:
        return vk::PipelineStageFlagBits::eTopOfPipe;
    case ResourceState::Common:
        return vk::PipelineStageFlagBits::eAllCommands;
    case ResourceState::Present:
        return vk::PipelineStageFlagBits::eBottomOfPipe;
    case ResourceState::RenderTarget:
        return vk::PipelineStageFlagBits::eColorAttachmentOutput;
    case ResourceState::InputAttachment:
        return vk::PipelineStageFlagBits::eFragmentShader;
    case ResourceState::IndirectArgument:
        return vk::PipelineStageFlagBits::eDrawIndirect;
    case ResourceState::VertexBuffer:
    case ResourceState::IndexBuffer:
        return vk::PipelineStageFlagBits::eVertexInput;
    case ResourceState::GenericRead:
    case ResourceState::ConstantBuffer:
    case ResourceState::UnorderedAccess:
    case ResourceState::ShaderResource:
        return vk::PipelineStageFlagBits::eVertexShader | // All possible shader stages
               vk::PipelineStageFlagBits::eFragmentShader;
    case ResourceState::CopyDest:
    case ResourceState::CopySource:
    case ResourceState::ResolveDest:
    case ResourceState::ResolveSource:
        return vk::PipelineStageFlagBits::eTransfer;
    case ResourceState::DepthWrite:
    case ResourceState::DepthRead:
        return vk::PipelineStageFlagBits::eEarlyFragmentTests |
               vk::PipelineStageFlagBits::eLateFragmentTests;
    case ResourceState::StreamOut:
        return {};
    default:
        META_UNEXPECTED_ARG_DESCR_RETURN(resource_state, vk::ImageLayout::eUndefined, "unexpected resource state");
    }
}

} // using namespace Methane::Graphics
