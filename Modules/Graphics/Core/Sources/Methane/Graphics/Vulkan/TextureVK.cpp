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

FILE: Methane/Graphics/Vulkan/TextureVK.mm
Vulkan implementation of the texture interface.

******************************************************************************/

#include "TextureVK.h"
#include "RenderCommandListVK.h"
#include "TypesVK.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <algorithm>

namespace Methane::Graphics
{

Ptr<Texture> Texture::CreateRenderTarget(const RenderContext& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage)
{
    META_FUNCTION_TASK();
    return std::make_shared<TextureVK>(dynamic_cast<const ContextBase&>(context), settings, descriptor_by_usage);
}

Ptr<Texture> Texture::CreateFrameBuffer(const RenderContext& context, uint32_t /*frame_buffer_index*/, const DescriptorByUsage& descriptor_by_usage)
{
    META_FUNCTION_TASK();
    const RenderContext::Settings& context_settings = context.GetSettings();
    const Settings texture_settings = Settings::FrameBuffer(Dimensions(context_settings.frame_size), context_settings.color_format);
    return std::make_shared<TextureVK>(dynamic_cast<const ContextBase&>(context), texture_settings, descriptor_by_usage);
}

Ptr<Texture> Texture::CreateDepthStencilBuffer(const RenderContext& context, const DescriptorByUsage& descriptor_by_usage)
{
    META_FUNCTION_TASK();
    const RenderContext::Settings& context_settings = context.GetSettings();
    const Settings texture_settings = Settings::DepthStencilBuffer(Dimensions(context_settings.frame_size), context_settings.depth_stencil_format);
    return std::make_shared<TextureVK>(dynamic_cast<const ContextBase&>(context), texture_settings, descriptor_by_usage);
}

Ptr<Texture> Texture::CreateImage(const Context& context, const Dimensions& dimensions, uint32_t array_length, PixelFormat pixel_format, bool mipmapped, const DescriptorByUsage& descriptor_by_usage)
{
    META_FUNCTION_TASK();
    const Settings texture_settings = Settings::Image(dimensions, array_length, pixel_format, mipmapped, Usage::ShaderRead);
    return std::make_shared<TextureVK>(dynamic_cast<const ContextBase&>(context), texture_settings, descriptor_by_usage);
}

Ptr<Texture> Texture::CreateCube(const Context& context, uint32_t dimension_size, uint32_t array_length, PixelFormat pixel_format, bool mipmapped, const DescriptorByUsage& descriptor_by_usage)
{
    META_FUNCTION_TASK();
    const Settings texture_settings = Settings::Cube(dimension_size, array_length, pixel_format, mipmapped, Usage::ShaderRead);
    return std::make_shared<TextureVK>(dynamic_cast<const ContextBase&>(context), texture_settings, descriptor_by_usage);
}

TextureVK::TextureVK(const ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage)
    : ResourceVK(context, settings, descriptor_by_usage)
{
    META_FUNCTION_TASK();
    InitializeDefaultDescriptors();
}

void TextureVK::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    ResourceVK::SetName(name);
}

void TextureVK::SetData(const SubResources& sub_resources, CommandQueue* sync_cmd_queue)
{
    META_FUNCTION_TASK();
    ResourceVK::SetData(sub_resources, sync_cmd_queue);
    
    if (GetSettings().mipmapped && sub_resources.size() < GetSubresourceCount().GetRawCount())
    {
        GenerateMipLevels();
    }
}

void TextureVK::UpdateFrameBuffer()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL_DESCR(GetSettings().type, Texture::Type::FrameBuffer, "unable to update frame buffer on non-FB texture");
}

void TextureVK::GenerateMipLevels()
{
    META_FUNCTION_TASK();
}

} // namespace Methane::Graphics
