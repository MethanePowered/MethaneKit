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

FILE: Methane/Graphics/Metal/TextureMT.mm
Metal implementation of the texture interface.

******************************************************************************/

#include "TextureMT.hh"
#include "RenderContextMT.hh"
#include "BlitCommandListMT.hh"
#include "TypesMT.hh"

#include <Methane/Instrumentation.h>
#include <Methane/Platform/MacOS/Types.hh>

#include <algorithm>
#include <cassert>

namespace Methane::Graphics
{

static MTLTextureType GetNativeTextureType(Texture::DimensionType dimension_type)
{
    ITT_FUNCTION_TASK();
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
    }
}

static MTLRegion GetTextureRegion(const Dimensions& dimensions, Texture::DimensionType dimension_type)
{
    ITT_FUNCTION_TASK();
    switch(dimension_type)
    {
    case Texture::DimensionType::Tex1D:
    case Texture::DimensionType::Tex1DArray:
            return MTLRegionMake1D(0, dimensions.width);
    case Texture::DimensionType::Tex2D:
    case Texture::DimensionType::Tex2DArray:
    case Texture::DimensionType::Tex2DMultisample:
    case Texture::DimensionType::Cube:
    case Texture::DimensionType::CubeArray:
            return MTLRegionMake2D(0, 0, dimensions.width, dimensions.height);
    case Texture::DimensionType::Tex3D:
            return MTLRegionMake3D(0, 0, 0, dimensions.width, dimensions.height, dimensions.depth);
    }
    return {};
}

Ptr<Texture> Texture::CreateRenderTarget(RenderContext& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<TextureMT>(dynamic_cast<ContextBase&>(context), settings, descriptor_by_usage);
}

Ptr<Texture> Texture::CreateFrameBuffer(RenderContext& context, uint32_t /*frame_buffer_index*/, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
    const RenderContext::Settings& context_settings = context.GetSettings();
    const Settings texture_settings = Settings::FrameBuffer(context_settings.frame_size, context_settings.color_format);
    return std::make_shared<TextureMT>(dynamic_cast<RenderContextBase&>(context), texture_settings, descriptor_by_usage);
}

Ptr<Texture> Texture::CreateDepthStencilBuffer(RenderContext& context, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
    const RenderContext::Settings& context_settings = context.GetSettings();
    const Settings texture_settings = Settings::DepthStencilBuffer(context_settings.frame_size, context_settings.depth_stencil_format);
    return std::make_shared<TextureMT>(dynamic_cast<RenderContextBase&>(context), texture_settings, descriptor_by_usage);
}

Ptr<Texture> Texture::CreateImage(Context& context, const Dimensions& dimensions, uint32_t array_length, PixelFormat pixel_format, bool mipmapped, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
    const Settings texture_settings = Settings::Image(dimensions, array_length, pixel_format, mipmapped, Usage::ShaderRead);
    return std::make_shared<TextureMT>(dynamic_cast<RenderContextBase&>(context), texture_settings, descriptor_by_usage);
}

Ptr<Texture> Texture::CreateCube(Context& context, uint32_t dimension_size, uint32_t array_length, PixelFormat pixel_format, bool mipmapped, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
    const Settings texture_settings = Settings::Cube(dimension_size, array_length, pixel_format, mipmapped, Usage::ShaderRead);
    return std::make_shared<TextureMT>(dynamic_cast<RenderContextBase&>(context), texture_settings, descriptor_by_usage);
}

TextureMT::TextureMT(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage)
    : TextureBase(context, settings, descriptor_by_usage)
    , m_mtl_texture(settings.type == Texture::Type::FrameBuffer
                      ? [GetRenderContextMT().GetNativeDrawable() texture]
                      : [GetContextMT().GetDeviceMT().GetNativeDevice()  newTextureWithDescriptor:GetNativeTextureDescriptor()])
{
    ITT_FUNCTION_TASK();

    InitializeDefaultDescriptors();
}

TextureMT::~TextureMT()
{
    ITT_FUNCTION_TASK();

    if (TextureBase::GetSettings().type != Texture::Type::FrameBuffer)
    {
        GetContext().GetResourceManager().GetReleasePool().AddResource(*this);
    }
}

void TextureMT::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();

    TextureBase::SetName(name);

    m_mtl_texture.label = MacOS::ConvertToNsType<std::string, NSString*>(name);
}

void TextureMT::SetData(const SubResources& sub_resources)
{
    ITT_FUNCTION_TASK();
    assert(m_mtl_texture != nil);

    if (sub_resources.empty())
    {
        throw std::invalid_argument("Can not set texture data from empty sub-resources.");
    }

    const Settings& settings        = GetSettings();
    const uint32_t  bytes_per_row   = settings.dimensions.width  * GetPixelSize(settings.pixel_format);
    const uint32_t  bytes_per_image = settings.dimensions.height * bytes_per_row;
    const MTLRegion texture_region  = GetTextureRegion(settings.dimensions, settings.dimension_type);

    for(const SubResource& sub_resource : sub_resources)
    {
        uint32_t slice = 0;
        switch(settings.dimension_type)
        {
            case Texture::DimensionType::Tex1DArray:
            case Texture::DimensionType::Tex2DArray:
                slice = sub_resource.index.array_index;
                break;
            case Texture::DimensionType::Cube:
                slice = sub_resource.index.depth_slice;
                break;
            case Texture::DimensionType::CubeArray:
                slice = sub_resource.index.depth_slice + sub_resource.index.array_index * 6;
                break;
            default:
                slice = 0;
        }
        
        [m_mtl_texture replaceRegion:texture_region
                         mipmapLevel:sub_resource.index.mip_level
                               slice:slice
                           withBytes:sub_resource.p_data
                         bytesPerRow:bytes_per_row
                       bytesPerImage:bytes_per_image];
    }

    if (settings.mipmapped && sub_resources.size() < GetRequiredSubresourceCount())
    {
        GenerateMipLevels();
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

    if (GetSettings().type != Texture::Type::FrameBuffer)
    {
        throw std::logic_error("Unable to update frame buffer on non-FB texture.");
    }

    m_mtl_texture = [GetRenderContextMT().GetNativeDrawable() texture];
}

MTLTextureUsage TextureMT::GetNativeTextureUsage()
{
    ITT_FUNCTION_TASK();

    NSUInteger texture_usage = MTLTextureUsageUnknown;
    const Settings& settings = GetSettings();
    
    if (settings.usage_mask & static_cast<uint32_t>(TextureBase::Usage::ShaderRead))
        texture_usage |= MTLTextureUsageShaderRead;
    
    if (settings.usage_mask & static_cast<uint32_t>(TextureBase::Usage::ShaderWrite))
        texture_usage |= MTLTextureUsageShaderWrite;
    
    if (settings.usage_mask & static_cast<uint32_t>(TextureBase::Usage::RenderTarget))
        texture_usage |= MTLTextureUsageRenderTarget;

    return texture_usage;
}

MTLTextureDescriptor* TextureMT::GetNativeTextureDescriptor()
{
    ITT_FUNCTION_TASK();

    const Settings& settings = GetSettings();
    const MTLPixelFormat mtl_pixel_format = TypeConverterMT::DataFormatToMetalPixelType(settings.pixel_format);
    const BOOL is_tex_mipmapped = MacOS::ConvertToNsType<bool, BOOL>(settings.mipmapped);

    MTLTextureDescriptor* mtl_tex_desc = nil;
    switch(settings.dimension_type)
    {
    case Texture::DimensionType::Tex2D:
        mtl_tex_desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:mtl_pixel_format
                                                                          width:settings.dimensions.width
                                                                         height:settings.dimensions.height
                                                                      mipmapped:is_tex_mipmapped];
        break;

    case Texture::DimensionType::Cube:
        mtl_tex_desc = [MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:mtl_pixel_format
                                                                             size:settings.dimensions.width
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
        mtl_tex_desc.textureType        = GetNativeTextureType(settings.dimension_type);
        mtl_tex_desc.width              = settings.dimensions.width;
        mtl_tex_desc.height             = settings.dimensions.height;
        mtl_tex_desc.depth              = settings.dimensions.depth;
        mtl_tex_desc.arrayLength        = settings.array_length;
        mtl_tex_desc.mipmapLevelCount   = GetMipLevelsCount();
        break;
    }

    if (!mtl_tex_desc)
        return nil;

    if (!settings.cpu_accessible)
    {
        mtl_tex_desc.resourceOptions = MTLResourceStorageModePrivate;
    }
    mtl_tex_desc.usage = GetNativeTextureUsage();

    return mtl_tex_desc;
}

void TextureMT::GenerateMipLevels()
{
    ITT_FUNCTION_TASK();

    BlitCommandListMT& blit_command_list = static_cast<BlitCommandListMT&>(GetContext().GetUploadCommandList());
    blit_command_list.Reset("Texture MIPs Generation");
    
    id<MTLBlitCommandEncoder>& mtl_blit_encoder = blit_command_list.GetNativeBlitEncoder();
    assert(mtl_blit_encoder != nil);
    assert(m_mtl_texture != nil);
    
    [mtl_blit_encoder generateMipmapsForTexture: m_mtl_texture];
}

RenderContextMT& TextureMT::GetRenderContextMT()
{
    ITT_FUNCTION_TASK();
    if (GetContext().GetType() != Context::Type::Render)
        throw std::runtime_error("Incompatible context type.");
    return static_cast<RenderContextMT&>(GetContextMT());
}

} // namespace Methane::Graphics
