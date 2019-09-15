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

FILE: Methane/Graphics/Metal/TextureMT.mm
Metal implementation of the texture interface.

******************************************************************************/

#include "TextureMT.hh"
#include "ContextMT.hh"
#include "DeviceMT.hh"
#include "TypesMT.hh"

#include <Methane/Instrumentation.h>
#include <Methane/Platform/MacOS/Types.hh>

#include <algorithm>
#include <cassert>

namespace Methane::Graphics
{

MTLTextureType GetNativeTextureType(Texture::DimensionType dimension_type)
{
    switch(dimension_type)
    {
    case Texture::DimensionType::Tex1D:             return MTLTextureType1D;
    case Texture::DimensionType::Tex1DArray:        return MTLTextureType1DArray;
    case Texture::DimensionType::Tex2D:             return MTLTextureType2D;
    case Texture::DimensionType::Tex2DArray:        return MTLTextureType2DArray;
    case Texture::DimensionType::Tex2DMultisample:  return MTLTextureType2DMultisample;
    // TODO: add support for MTLTextureType2DMultisampleArray
    case Texture::DimensionType::Cube:              return MTLTextureTypeCube;
    case Texture::DimensionType::CubeArray:         return MTLTextureTypeCubeArray;
    case Texture::DimensionType::Tex3D:             return MTLTextureType3D;
    // TODO: add support for MTLTextureTypeTextureBuffer
    default: throw std::invalid_argument("Dimension type is not supported in Metal");
    }
}

Texture::Ptr Texture::CreateRenderTarget(Context& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<TextureMT>(static_cast<ContextBase&>(context), settings, descriptor_by_usage);
}

Texture::Ptr Texture::CreateFrameBuffer(Context& context, uint32_t /*frame_buffer_index*/, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
    const Context::Settings& context_settings = context.GetSettings();
    const Settings texture_settings = Settings::FrameBuffer(context_settings.frame_size, context_settings.color_format);
    return std::make_shared<TextureMT>(static_cast<ContextBase&>(context), texture_settings, descriptor_by_usage);
}

Texture::Ptr Texture::CreateDepthStencilBuffer(Context& context, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
    const Context::Settings& context_settings = context.GetSettings();
    const Settings texture_settings = Settings::DepthStencilBuffer(context_settings.frame_size, context_settings.depth_stencil_format);
    return std::make_shared<TextureMT>(static_cast<ContextBase&>(context), texture_settings, descriptor_by_usage);
}

Texture::Ptr Texture::CreateImage(Context& context, Dimensions dimensions, uint32_t array_length, PixelFormat pixel_format, bool mipmapped, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
    const Settings texture_settings = Settings::Image(dimensions, array_length, pixel_format, mipmapped, Usage::ShaderRead);
    return std::make_shared<TextureMT>(static_cast<ContextBase&>(context), texture_settings, descriptor_by_usage);
}

Texture::Ptr Texture::CreateCube(Context& context, uint32_t dimension_size, uint32_t array_length, PixelFormat pixel_format, bool mipmapped, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
    const Settings texture_settings = Settings::Cube(dimension_size, array_length, pixel_format, mipmapped, Usage::ShaderRead);
    return std::make_shared<TextureMT>(static_cast<ContextBase&>(context), texture_settings, descriptor_by_usage);
}

TextureMT::TextureMT(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage)
    : TextureBase(context, settings, descriptor_by_usage)
    , m_mtl_texture(settings.type == Texture::Type::FrameBuffer
                      ? [GetContextMT().GetNativeDrawable() texture]
                      : [GetContextMT().GetDeviceMT().GetNativeDevice()  newTextureWithDescriptor:GetNativeTextureDescriptor()])
{
    ITT_FUNCTION_TASK();

    InitializeDefaultDescriptors();
}

TextureMT::~TextureMT()
{
    ITT_FUNCTION_TASK();

    if (m_settings.type != Texture::Type::FrameBuffer)
    {
        m_context.GetResourceManager().GetReleasePool().AddResource(*this);
    }
}

void TextureMT::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();

    TextureBase::SetName(name);

    m_mtl_texture.label = MacOS::ConvertToNSType<std::string, NSString*>(name);
}

void TextureMT::SetData(const SubResources& sub_resources)
{
    ITT_FUNCTION_TASK();
    assert(m_mtl_texture != nil);

    if (sub_resources.empty())
    {
        throw std::invalid_argument("Can not set texture data from empty sub-resources.");
    }
    
    const uint32_t bytes_per_row   = m_settings.dimensions.width  * GetPixelSize(m_settings.pixel_format);
    const uint32_t bytes_per_image = m_settings.dimensions.height * bytes_per_row;
    const MTLRegion texture_region = MTLRegionMake2D(0, 0, m_settings.dimensions.width, m_settings.dimensions.height);

    for(const SubResource& sub_resourse : sub_resources)
    {
        [m_mtl_texture replaceRegion:texture_region
                         mipmapLevel:sub_resourse.mip_level
                               slice:sub_resourse.depth_slice
                           withBytes:sub_resourse.p_data
                         bytesPerRow:bytes_per_row
                       bytesPerImage:bytes_per_image];
    }
}

Data::Size TextureMT::GetDataSize() const
{
    ITT_FUNCTION_TASK();
    throw std::logic_error("Getting of texture data size is not implemented.");
}

void TextureMT::UpdateFrameBuffer()
{
    ITT_FUNCTION_TASK();

    if (m_settings.type != Texture::Type::FrameBuffer)
    {
        throw std::logic_error("Unable to update frame buffer on non-FB texture.");
    }

    m_mtl_texture = [GetContextMT().GetNativeDrawable() texture];
}

MTLTextureUsage TextureMT::GetNativeTextureUsage()
{
    ITT_FUNCTION_TASK();

    NSUInteger texture_usage = MTLTextureUsageUnknown;
    
    if (m_settings.usage_mask & static_cast<uint32_t>(TextureBase::Usage::ShaderRead))
        texture_usage |= MTLTextureUsageShaderRead;
    
    if (m_settings.usage_mask & static_cast<uint32_t>(TextureBase::Usage::ShaderWrite))
        texture_usage |= MTLTextureUsageShaderWrite;
    
    if (m_settings.usage_mask & static_cast<uint32_t>(TextureBase::Usage::RenderTarget))
        texture_usage |= MTLTextureUsageRenderTarget;

    return texture_usage;
}

MTLTextureDescriptor* TextureMT::GetNativeTextureDescriptor()
{
    ITT_FUNCTION_TASK();

    const MTLPixelFormat mtl_pixel_format = TypeConverterMT::DataFormatToMetalPixelType(m_settings.pixel_format);
    const BOOL is_tex_mipmapped = MacOS::ConvertToNSType<bool, BOOL>(m_settings.mipmapped);

    MTLTextureDescriptor* mtl_tex_desc = nil;
    switch(m_settings.dimension_type)
    {
    case Texture::DimensionType::Tex2D:
        mtl_tex_desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:mtl_pixel_format
                                                                          width:m_settings.dimensions.width
                                                                         height:m_settings.dimensions.height
                                                                      mipmapped:is_tex_mipmapped];
        break;

    case Texture::DimensionType::Cube:
        mtl_tex_desc = [MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:mtl_pixel_format
                                                                             size:m_settings.dimensions.width
                                                                        mipmapped:is_tex_mipmapped];
        break;

    case Texture::DimensionType::Tex1D:
    case Texture::DimensionType::Tex1DArray:
    case Texture::DimensionType::Tex2DArray:
    case Texture::DimensionType::Tex2DMultisample:
    case Texture::DimensionType::CubeArray:
    case Texture::DimensionType::Tex3D:
        mtl_tex_desc                    = [[MTLTextureDescriptor alloc] init];
        mtl_tex_desc.pixelFormat        = mtl_pixel_format;
        mtl_tex_desc.textureType        = GetNativeTextureType(m_settings.dimension_type);
        mtl_tex_desc.width              = m_settings.dimensions.width;
        mtl_tex_desc.height             = m_settings.dimensions.height;
        mtl_tex_desc.depth              = m_settings.dimensions.depth;
        mtl_tex_desc.arrayLength        = m_settings.array_length;
        mtl_tex_desc.mipmapLevelCount   = 1; // TODO: calculate mip-map level count
        break;

    default:
        throw std::logic_error("Unsupported texture dimension type in Metal");
    }

    if (!mtl_tex_desc)
        return nil;

    if (!m_settings.cpu_accessible)
    {
        mtl_tex_desc.resourceOptions = MTLResourceStorageModePrivate;
    }
    mtl_tex_desc.usage = GetNativeTextureUsage();

    return mtl_tex_desc;
}

} // namespace Methane::Graphics
