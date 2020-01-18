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

FILE: Methane/Graphics/Vulkan/TextureVK.mm
Vulkan implementation of the texture interface.

******************************************************************************/

#include "TextureVK.h"
#include "ContextVK.h"
#include "RenderCommandListVK.h"
#include "TypesVK.h"

#include <Methane/Data/Instrumentation.h>

#include <algorithm>
#include <cassert>

namespace Methane::Graphics
{

Ptr<Texture> Texture::CreateRenderTarget(Context& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<TextureVK>(static_cast<ContextBase&>(context), settings, descriptor_by_usage);
}

Ptr<Texture> Texture::CreateFrameBuffer(Context& context, uint32_t /*frame_buffer_index*/, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
    const Context::Settings& context_settings = context.GetSettings();
    const Settings texture_settings = Settings::FrameBuffer(context_settings.frame_size, context_settings.color_format);
    return std::make_shared<TextureVK>(static_cast<ContextBase&>(context), texture_settings, descriptor_by_usage);
}

Ptr<Texture> Texture::CreateDepthStencilBuffer(Context& context, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
    const Context::Settings& context_settings = context.GetSettings();
    const Settings texture_settings = Settings::DepthStencilBuffer(context_settings.frame_size, context_settings.depth_stencil_format);
    return std::make_shared<TextureVK>(static_cast<ContextBase&>(context), texture_settings, descriptor_by_usage);
}

Ptr<Texture> Texture::CreateImage(Context& context, const Dimensions& dimensions, uint32_t array_length, PixelFormat pixel_format, bool mipmapped, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
    const Settings texture_settings = Settings::Image(dimensions, array_length, pixel_format, mipmapped, Usage::ShaderRead);
    return std::make_shared<TextureVK>(static_cast<ContextBase&>(context), texture_settings, descriptor_by_usage);
}

Ptr<Texture> Texture::CreateCube(Context& context, uint32_t dimension_size, uint32_t array_length, PixelFormat pixel_format, bool mipmapped, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
    const Settings texture_settings = Settings::Cube(dimension_size, array_length, pixel_format, mipmapped, Usage::ShaderRead);
    return std::make_shared<TextureVK>(static_cast<ContextBase&>(context), texture_settings, descriptor_by_usage);
}

TextureVK::TextureVK(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage)
    : TextureBase(context, settings, descriptor_by_usage)
{
    ITT_FUNCTION_TASK();

    InitializeDefaultDescriptors();
}

TextureVK::~TextureVK()
{
    ITT_FUNCTION_TASK();

    if (m_settings.type != Texture::Type::FrameBuffer)
    {
        m_context.GetResourceManager().GetReleasePool().AddResource(*this);
    }
}

void TextureVK::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();

    TextureBase::SetName(name);
}

void TextureVK::SetData(const SubResources& sub_resources)
{
    ITT_FUNCTION_TASK();

    if (sub_resources.empty())
    {
        throw std::invalid_argument("Can not set texture data from empty sub-resources.");
    }
    
    if (m_settings.mipmapped && sub_resources.size() < GetRequiredSubresourceCount())
    {
        GenerateMipLevels();
    }
}

Data::Size TextureVK::GetDataSize() const
{
    ITT_FUNCTION_TASK();
    throw std::logic_error("Getting of texture data size is not implemented.");
}

void TextureVK::UpdateFrameBuffer()
{
    ITT_FUNCTION_TASK();

    if (m_settings.type != Texture::Type::FrameBuffer)
    {
        throw std::logic_error("Unable to update frame buffer on non-FB texture.");
    }
}

void TextureVK::GenerateMipLevels()
{
    ITT_FUNCTION_TASK();
}

} // namespace Methane::Graphics
