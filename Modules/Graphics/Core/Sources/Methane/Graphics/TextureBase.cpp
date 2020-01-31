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

FILE: Methane/Graphics/TextureBase.cpp
Base implementation of the texture interface.

******************************************************************************/

#include "TextureBase.h"
#include "DescriptorHeap.h"
#include "RenderContextBase.h"

#include <Methane/Instrumentation.h>

#include <cassert>

namespace Methane::Graphics
{

Texture::Settings Texture::Settings::Image(const Dimensions& dimensions, uint32_t array_length, PixelFormat pixel_format, bool mipmapped, TextureBase::Usage::Mask usage)
{
    ITT_FUNCTION_TASK();

    Settings settings;
    settings.type           = Type::Texture;
    settings.dimension_type = dimensions.height == 1
                            ? (array_length == 1 ? DimensionType::Tex1D : DimensionType::Tex1DArray)
                            : (dimensions.depth == 1
                                ? (array_length == 1 ? DimensionType::Tex2D : DimensionType::Tex2DArray)
                                : DimensionType::Tex3D);
    settings.dimensions     = dimensions;
    settings.array_length   = array_length;
    settings.pixel_format   = pixel_format;
    settings.usage_mask     = usage;
    settings.mipmapped      = mipmapped;
    settings.cpu_accessible = true;

    return settings;
}

Texture::Settings Texture::Settings::Cube(uint32_t dimension_size, uint32_t array_length, PixelFormat pixel_format, bool mipmapped, Usage::Mask usage)
{
    ITT_FUNCTION_TASK();

    Settings settings;
    settings.type           = Type::Texture;
    settings.dimension_type = array_length == 1 ? DimensionType::Cube : DimensionType::CubeArray;
    settings.dimensions     = Dimensions(dimension_size, dimension_size, 6);
    settings.array_length   = array_length;
    settings.pixel_format   = pixel_format;
    settings.usage_mask     = usage;
    settings.mipmapped      = mipmapped;
    settings.cpu_accessible = true;

    return settings;
}

Texture::Settings Texture::Settings::FrameBuffer(const Dimensions& dimensions, PixelFormat pixel_format)
{
    ITT_FUNCTION_TASK();

    Settings settings;
    settings.type           = Type::FrameBuffer;
    settings.dimension_type = DimensionType::Tex2D;
    settings.usage_mask     = Usage::RenderTarget;
    settings.pixel_format   = pixel_format;
    settings.dimensions     = dimensions;
    settings.cpu_accessible = false;

    return settings;
}

Texture::Settings Texture::Settings::DepthStencilBuffer(const Dimensions& dimensions, PixelFormat pixel_format, Usage::Mask usage_mask)
{
    ITT_FUNCTION_TASK();

    Settings settings;
    settings.type           = Type::DepthStencilBuffer;
    settings.dimension_type = DimensionType::Tex2D;
    settings.usage_mask     = usage_mask;
    settings.pixel_format   = pixel_format;
    settings.dimensions     = dimensions;
    settings.cpu_accessible = false;
    return settings;
}

TextureBase::TextureBase(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage)
    : ResourceNT(Resource::Type::Texture, settings.usage_mask, context, descriptor_by_usage)
    , m_settings(settings)
{
    ITT_FUNCTION_TASK();

    if (m_settings.usage_mask == TextureBase::Usage::Unknown)
    {
        throw std::invalid_argument("Can not create texture with \"Unknown\" usage mask.");
    }
    if (m_settings.pixel_format == PixelFormat::Unknown)
    {
        throw std::invalid_argument("Can not create texture with \"Unknown\" pixel format.");
    }
    if (!m_settings.array_length)
    {
        throw std::invalid_argument("Array length should be greater than zero.");
    }

    ValidateDimensions(m_settings.dimension_type, m_settings.dimensions, m_settings.mipmapped);
}

void TextureBase::ValidateDimensions(DimensionType dimension_type, const Dimensions& dimensions, bool mipmapped)
{
    ITT_FUNCTION_TASK();

    if (!dimensions.width || !dimensions.height || !dimensions.depth)
    {
        throw std::invalid_argument("All dimension sizes should be greater than zero.");
    }

    switch (dimension_type)
    {
    case DimensionType::Cube:
    case DimensionType::CubeArray:
        if (dimensions.width != dimensions.height)
        {
            throw std::invalid_argument("Cube texture must have equal width and height dimensions.");
        }
        if (dimensions.depth != 6)
        {
            throw std::invalid_argument("Cube texture depth must be equal to 6.");
        }
        // NOTE: break is missing intentionally
    case DimensionType::Tex3D:
        if (mipmapped && dimensions.depth % 2)
        {
            throw std::invalid_argument("All dimensions of the mip-mapped texture should be a power of 2, but depth is not.");
        }
        // NOTE: break is missing intentionally
    case DimensionType::Tex2D:
    case DimensionType::Tex2DArray:
    case DimensionType::Tex2DMultisample:
        if (mipmapped && dimensions.height % 2)
        {
            throw std::invalid_argument("All dimensions of the mip-mapped texture should be a power of 2, but height is not.");
        }
        // NOTE: break is missing intentionally
    case DimensionType::Tex1D:
    case DimensionType::Tex1DArray:
        if (mipmapped && dimensions.width % 2)
        {
            throw std::invalid_argument("All dimensions of the mip-mapped texture should be a power of 2, but width is not.");
        }
        // NOTE: break is missing intentionally
    default: return;
    }
}

uint32_t TextureBase::GetMipLevelsCount() const
{
    ITT_FUNCTION_TASK();
    return m_settings.mipmapped ? 1u + static_cast<uint32_t>(std::log2(static_cast<double>(m_settings.dimensions.GetLongestSide())))
                                : 1u;
}

uint32_t TextureBase::GetRequiredSubresourceCount() const
{
    ITT_FUNCTION_TASK();
    return m_settings.array_length * m_settings.dimensions.depth * GetMipLevelsCount();
}

} // namespace Methane::Graphics