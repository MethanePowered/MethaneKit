/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/ITexture.cpp
Methane graphics interface: graphics texture.

******************************************************************************/

#include <Methane/Graphics/RHI/ITexture.h>
#include <Methane/Graphics/RHI/IRenderContext.h>

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

TextureView::TextureView(ITexture& texture, const SubResource::Index& subresource_index, const SubResource::Count& subresource_count,
                         Opt<TextureDimensionType> texture_dimension_type_opt)
    : ResourceView(texture, subresource_index, subresource_count, texture_dimension_type_opt)
    , m_texture_ptr(std::dynamic_pointer_cast<ITexture>(GetResourcePtr()))
{ }

Rhi::ITexture& TextureView::GetTexture() const
{
    META_CHECK_ARG_NOT_NULL_DESCR(m_texture_ptr, "can not get texture from uninitialized resource view");
    return *m_texture_ptr;
}

TextureSettings TextureSettings::ForImage(const Dimensions& dimensions, const Opt<uint32_t>& array_length_opt, PixelFormat pixel_format,
                                          bool mipmapped, ResourceUsageMask usage)
{
    META_FUNCTION_TASK();

    TextureSettings settings;
    if (dimensions.GetHeight() == 1)
        settings.dimension_type = array_length_opt ? TextureDimensionType::Tex1DArray : TextureDimensionType::Tex1D;
    else if (dimensions.GetDepth() == 1)
        settings.dimension_type = array_length_opt ? TextureDimensionType::Tex2DArray : TextureDimensionType::Tex2D;
    else
        settings.dimension_type = TextureDimensionType::Tex3D;
    settings.type         = usage.HasAnyBit(ResourceUsage::RenderTarget) ? TextureType::RenderTarget : TextureType::Image;
    settings.dimensions   = dimensions;
    settings.array_length = array_length_opt.value_or(1U);
    settings.pixel_format = pixel_format;
    settings.usage_mask   = usage;
    settings.mipmapped    = mipmapped;

    return settings;
}

TextureSettings TextureSettings::ForCubeImage(uint32_t dimension_size, const Opt<uint32_t>& array_length_opt, PixelFormat pixel_format,
                                              bool mipmapped, ResourceUsageMask usage)
{
    META_FUNCTION_TASK();

    TextureSettings settings;
    settings.type           = usage.HasAnyBit(ResourceUsage::RenderTarget) ? TextureType::RenderTarget : TextureType::Image;
    settings.dimension_type = array_length_opt ? TextureDimensionType::CubeArray : TextureDimensionType::Cube;
    settings.dimensions     = Dimensions(dimension_size, dimension_size, 6U);
    settings.array_length   = array_length_opt.value_or(1U);
    settings.pixel_format   = pixel_format;
    settings.usage_mask     = usage;
    settings.mipmapped      = mipmapped;

    return settings;
}

TextureSettings TextureSettings::ForFrameBuffer(const Dimensions& dimensions, PixelFormat pixel_format, Data::Index frame_index)
{
    META_FUNCTION_TASK();

    TextureSettings settings;
    settings.type            = TextureType::FrameBuffer;
    settings.dimension_type  = TextureDimensionType::Tex2D;
    settings.usage_mask      = ResourceUsageMask(ResourceUsage::RenderTarget);
    settings.pixel_format    = pixel_format;
    settings.dimensions      = dimensions;
    settings.frame_index_opt = frame_index;

    return settings;
}

TextureSettings TextureSettings::ForFrameBuffer(const RenderContextSettings& render_context_settings, Data::Index frame_index)
{
    META_FUNCTION_TASK();
    return ForFrameBuffer(Dimensions(render_context_settings.frame_size), render_context_settings.color_format, frame_index);
}

TextureSettings TextureSettings::ForDepthStencil(const Dimensions& dimensions, PixelFormat pixel_format,
                                                 const Opt<DepthStencilValues>& depth_stencil_clear,
                                                 ResourceUsageMask usage_mask)
{
    META_FUNCTION_TASK();

    TextureSettings settings;
    settings.type                    = TextureType::DepthStencil;
    settings.dimension_type          = TextureDimensionType::Tex2D;
    settings.usage_mask              = usage_mask;
    settings.pixel_format            = pixel_format;
    settings.dimensions              = dimensions;
    settings.depth_stencil_clear_opt = depth_stencil_clear;

    return settings;
}

TextureSettings TextureSettings::ForDepthStencil(const RenderContextSettings& render_context_settings)
{
    META_FUNCTION_TASK();
    return ForDepthStencil(Dimensions(render_context_settings.frame_size), render_context_settings.depth_stencil_format,
                           render_context_settings.clear_depth_stencil);
}

Ptr<ITexture> ITexture::Create(const IContext& context, const Settings& settings)
{
    META_FUNCTION_TASK();
    return context.CreateTexture(settings);
}

} // namespace Methane::Graphics::Rhi
