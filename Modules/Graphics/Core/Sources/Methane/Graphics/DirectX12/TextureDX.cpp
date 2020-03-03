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

FILE: Methane/Graphics/DirectX12/TextureDX.cpp
DirectX 12 implementation of the texture interface.

******************************************************************************/

#include "TextureDX.h"
#include "RenderContextDX.h"
#include "DeviceDX.h"
#include "DescriptorHeapDX.h"
#include "CommandQueueDX.h"
#include "BlitCommandListDX.h"
#include "TypesDX.h"

#include <Methane/Instrumentation.h>
#include <Methane/Graphics/Windows/Helpers.h>

#include <DirectXTex.h>

namespace Methane::Graphics
{

static D3D12_SRV_DIMENSION GetSrvDimension(const Dimensions& tex_dimensions) noexcept
{
    ITT_FUNCTION_TASK();
    return tex_dimensions.depth == 1 ? (tex_dimensions.height == 1 ? D3D12_SRV_DIMENSION_TEXTURE1D
                                                                   : D3D12_SRV_DIMENSION_TEXTURE2D)
                                     : D3D12_SRV_DIMENSION_TEXTURE3D;
}

static D3D12_DSV_DIMENSION GetDsvDimension(const Dimensions& tex_dimensions)
{
    ITT_FUNCTION_TASK();
    if (tex_dimensions.depth != 1)
    {
        throw std::runtime_error("Depth-stencil view can not be created for 3D texture.");
    }
    return  tex_dimensions.height == 1 ? D3D12_DSV_DIMENSION_TEXTURE1D
                                       : D3D12_DSV_DIMENSION_TEXTURE2D;
}

Ptr<Texture> Texture::CreateRenderTarget(RenderContext& render_context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
    
    switch (settings.type)
    {
    case Texture::Type::FrameBuffer:        throw std::logic_error("Frame buffer texture must be created with static method Texture::CreateFrameBuffer.");
    case Texture::Type::DepthStencilBuffer: return std::make_shared<DepthStencilBufferTextureDX>(static_cast<RenderContextBase&>(render_context), settings, descriptor_by_usage, render_context.GetSettings().clear_depth_stencil);
    default:                                return std::make_shared<RenderTargetTextureDX>(static_cast<RenderContextBase&>(render_context), settings, descriptor_by_usage);
    }
}

Ptr<Texture> Texture::CreateFrameBuffer(RenderContext& render_context, uint32_t frame_buffer_index, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
    
    const RenderContext::Settings& context_settings = render_context.GetSettings();
    const Settings texture_settings = Settings::FrameBuffer(context_settings.frame_size, context_settings.color_format);
    return std::make_shared<FrameBufferTextureDX>(static_cast<RenderContextBase&>(render_context), texture_settings, descriptor_by_usage, frame_buffer_index);
}

Ptr<Texture> Texture::CreateDepthStencilBuffer(RenderContext& render_context, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
    
    const RenderContext::Settings& context_settings = render_context.GetSettings();
    const Settings texture_settings = Settings::DepthStencilBuffer(context_settings.frame_size, context_settings.depth_stencil_format);
    return std::make_shared<DepthStencilBufferTextureDX>(static_cast<RenderContextBase&>(render_context), texture_settings, descriptor_by_usage, context_settings.clear_depth_stencil);
}

Ptr<Texture> Texture::CreateImage(Context& render_context, const Dimensions& dimensions, uint32_t array_length, PixelFormat pixel_format, bool mipmapped, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
    const Settings texture_settings = Settings::Image(dimensions, array_length, pixel_format, mipmapped, Usage::ShaderRead);
    return std::make_shared<ImageTextureDX>(dynamic_cast<ContextBase&>(render_context), texture_settings, descriptor_by_usage, ImageTextureArg());
}

Ptr<Texture> Texture::CreateCube(Context& render_context, uint32_t dimension_size, uint32_t array_length, PixelFormat pixel_format, bool mipmapped, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
    const Settings texture_settings = Settings::Cube(dimension_size, array_length, pixel_format, mipmapped, Usage::ShaderRead);
    return std::make_shared<ImageTextureDX>(dynamic_cast<ContextBase&>(render_context), texture_settings, descriptor_by_usage, ImageTextureArg());
}

template<>
void RenderTargetTextureDX::Initialize()
{
    ITT_FUNCTION_TASK();

    const Settings& settings = GetSettings();
    if (settings.dimensions.depth != 1 || settings.dimensions.width == 0 || settings.dimensions.height == 0)
    {
        throw std::invalid_argument("Render target texture can only be created for 2D texture with dimensions.depth == 1 and non zero width and height.");
    }

    D3D12_RESOURCE_DESC tex_desc = CD3DX12_RESOURCE_DESC::Tex2D(
        TypeConverterDX::DataFormatToDXGI(settings.pixel_format),
        settings.dimensions.width,
        settings.dimensions.height
    );
    InitializeCommittedResource(tex_desc, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_GENERIC_READ);
}

template<>
void FrameBufferTextureDX::Initialize(uint32_t frame_buffer_index)
{
    ITT_FUNCTION_TASK();

    InitializeFrameBufferResource(frame_buffer_index);

    if (GetUsageMask() != Usage::RenderTarget)
    {
        throw std::runtime_error("Frame BufferBase texture supports only Render Target usage.");
    }

    const D3D12_CPU_DESCRIPTOR_HANDLE descriptor_handle = GetNativeCpuDescriptorHandle(Usage::RenderTarget);
    GetContextDX().GetDeviceDX().GetNativeDevice()->CreateRenderTargetView(GetNativeResource(), nullptr, descriptor_handle);
}

template<>
void DepthStencilBufferTextureDX::Initialize(const std::optional<DepthStencil>& clear_depth_stencil)
{
    ITT_FUNCTION_TASK();

    const Settings& settings = GetSettings();
    CD3DX12_RESOURCE_DESC tex_desc = CD3DX12_RESOURCE_DESC::Tex2D(
        TypeConverterDX::DataFormatToDXGI(settings.pixel_format, TypeConverterDX::ResourceFormatType::ResourceBase),
        settings.dimensions.width, settings.dimensions.height
    );

    tex_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    if (settings.usage_mask & Usage::RenderTarget)
    {
        tex_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }
    if (!(settings.usage_mask & Usage::ShaderRead ||
          settings.usage_mask & Usage::ShaderWrite))
    {
        tex_desc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    }

    const DXGI_FORMAT view_write_format = TypeConverterDX::DataFormatToDXGI(settings.pixel_format, TypeConverterDX::ResourceFormatType::ViewWrite);

    if (clear_depth_stencil)
    {
        // Performance tip: Tell the runtime at resource creation the desired clear value
        CD3DX12_CLEAR_VALUE clear_value(view_write_format, clear_depth_stencil->first, clear_depth_stencil->second);
        InitializeCommittedResource(tex_desc, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear_value);
    }
    else
    {
        InitializeCommittedResource(tex_desc, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    }

    const wrl::ComPtr<ID3D12Device>& cp_device = GetContextDX().GetDeviceDX().GetNativeDevice();

    const Usage::Mask usage_mask = GetUsageMask();
    for (Usage::Value usage : Usage::primary_values)
    {
        if (!(usage_mask & usage))
            continue;

        const Descriptor& desc = ResourceBase::GetDescriptorByUsage(usage);
        switch (desc.heap.GetSettings().type)
        {
        case DescriptorHeap::Type::ShaderResources:
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
            srv_desc.Format                          = TypeConverterDX::DataFormatToDXGI(settings.pixel_format, TypeConverterDX::ResourceFormatType::ViewRead);
            srv_desc.ViewDimension                   = GetSrvDimension(settings.dimensions);
            srv_desc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srv_desc.Texture2D.MipLevels             = 1;
            cp_device->CreateShaderResourceView(GetNativeResource(), &srv_desc, GetNativeCpuDescriptorHandle(desc));
        } break;

        case DescriptorHeap::Type::DepthStencil:
        {
            D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc   = {};
            dsv_desc.Format                          = view_write_format;
            dsv_desc.ViewDimension                   = GetDsvDimension(settings.dimensions);
            cp_device->CreateDepthStencilView(GetNativeResource(), &dsv_desc, GetNativeCpuDescriptorHandle(desc));
        } break;

        default:
            throw std::runtime_error("Unsupported usage of Depth-Stencil buffer.");
        }
    }
}

ImageTextureDX::TextureDX(ContextBase& render_context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage, ImageTextureArg)
    : TextureBase(render_context, settings, descriptor_by_usage)
{
    ITT_FUNCTION_TASK();

    if (ResourceBase::GetUsageMask() != Usage::ShaderRead)
    {
        throw std::runtime_error("Image texture supports only \"Shader Read\" usage.");
    }

    InitializeDefaultDescriptors();

    const ResourceAndViewDesc tex_and_srv_desc = GetResourceAndViewDesc();
    InitializeCommittedResource(tex_and_srv_desc.first, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COPY_DEST);

    const wrl::ComPtr<ID3D12Device>& cp_device = GetContextDX().GetDeviceDX().GetNativeDevice();
    const UINT number_of_subresources = GetRequiredSubresourceCount();
    const UINT64 upload_buffer_size   = GetRequiredIntermediateSize(GetNativeResource(), 0, number_of_subresources);
    ThrowIfFailed(
        cp_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(upload_buffer_size),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_cp_upload_resource)
        )
    );

    cp_device->CreateShaderResourceView(GetNativeResource(), &tex_and_srv_desc.second, GetNativeCpuDescriptorHandle(Usage::ShaderRead));
}

ImageTextureDX::ResourceAndViewDesc ImageTextureDX::GetResourceAndViewDesc() const
{
    const Settings& settings = GetSettings();
    assert(settings.dimensions.depth  > 0);
    assert(settings.dimensions.width  > 0);
    assert(settings.dimensions.height > 0);

    D3D12_RESOURCE_DESC tex_desc = {};
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    const uint32_t mip_levels_count = GetMipLevelsCount();

    switch (settings.dimension_type)
    {
    case DimensionType::Tex1D:
        if (settings.array_length != 1)
        {
            throw std::invalid_argument("Single 1D Texture must have array length equal to 1.");
        }
        srv_desc.Texture1D.MipLevels = mip_levels_count;
        // NOTE: break is missing intentionally

    case DimensionType::Tex1DArray:
        if (settings.dimensions.height != 1 || settings.dimensions.depth != 1)
        {
            throw std::invalid_argument("1D Textures must have height and depth dimensions equal to 1.");
        }

        tex_desc = CD3DX12_RESOURCE_DESC::Tex1D(
            TypeConverterDX::DataFormatToDXGI(settings.pixel_format),
            settings.dimensions.width,
            settings.array_length,
            mip_levels_count
        );

        srv_desc.Texture1DArray.MipLevels   = mip_levels_count;
        srv_desc.Texture1DArray.ArraySize   = settings.array_length;
        srv_desc.ViewDimension              = settings.dimension_type == DimensionType::Tex1D
                                            ? D3D12_SRV_DIMENSION_TEXTURE1D
                                            : D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
        break;

    case DimensionType::Tex2D:
        if (settings.array_length != 1)
        {
            throw std::invalid_argument("Single 2D Texture must have array length equal to 1.");
        }
        srv_desc.Texture2D.MipLevels = mip_levels_count;
        // NOTE: break is missing intentionally

    case DimensionType::Tex2DArray:
        if (settings.dimensions.depth != 1)
        {
            throw std::invalid_argument("2D Textures must have depth dimension equal to 1.");
        }
        tex_desc = CD3DX12_RESOURCE_DESC::Tex2D(
            TypeConverterDX::DataFormatToDXGI(settings.pixel_format),
            settings.dimensions.width,
            settings.dimensions.height,
            settings.array_length,
            mip_levels_count
        );

        srv_desc.Texture2DArray.MipLevels   = mip_levels_count;
        srv_desc.Texture2DArray.ArraySize   = settings.array_length;
        srv_desc.ViewDimension              = settings.dimension_type == DimensionType::Tex2D
                                            ? D3D12_SRV_DIMENSION_TEXTURE2D
                                            : D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        break;

    case DimensionType::Tex3D:
        if (settings.array_length != 1)
        {
            throw std::invalid_argument("Single 3D Texture must have array length equal to 1.");
        }
        tex_desc = CD3DX12_RESOURCE_DESC::Tex3D(
            TypeConverterDX::DataFormatToDXGI(settings.pixel_format),
            settings.dimensions.width,
            settings.dimensions.height,
            settings.dimensions.depth,
            mip_levels_count
        );
        srv_desc.Texture3D.MipLevels = mip_levels_count;
        srv_desc.ViewDimension       = D3D12_SRV_DIMENSION_TEXTURE3D;
        break;

    case DimensionType::Cube:
        if (settings.array_length != 1)
        {
            throw std::invalid_argument("Single Cube Texture must have array length equal to 1.");
        }
        srv_desc.TextureCube.MipLevels = mip_levels_count;
        // NOTE: break is missing intentionally

    case DimensionType::CubeArray:
        if (settings.dimensions.depth != 6)
        {
            throw std::invalid_argument("Cube textures must have depth dimension equal to 6.");
        }
        tex_desc = CD3DX12_RESOURCE_DESC::Tex2D(
            TypeConverterDX::DataFormatToDXGI(settings.pixel_format),
            settings.dimensions.width,
            settings.dimensions.height,
            settings.dimensions.depth * settings.array_length,
            mip_levels_count
        );

        srv_desc.TextureCubeArray.First2DArrayFace = 0;
        srv_desc.TextureCubeArray.NumCubes         = settings.array_length;
        srv_desc.TextureCubeArray.MipLevels        = mip_levels_count;
        srv_desc.ViewDimension                     = settings.dimension_type == DimensionType::Cube
                                                   ? D3D12_SRV_DIMENSION_TEXTURECUBE
                                                   : D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        break;
    }

    srv_desc.Format                  = tex_desc.Format;
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    return { std::move(tex_desc), std::move(srv_desc) };
}

void ImageTextureDX::SetData(const SubResources& sub_resources)
{
    ITT_FUNCTION_TASK();

    if (sub_resources.empty())
    {
        throw std::invalid_argument("Can not set texture data from empty sub-resources.");
    }

    m_data_size = 0;
    
    assert(!!m_cp_upload_resource);

    const Settings&  settings                    = GetSettings();
    const Data::Size pixel_size                  = GetPixelSize(settings.pixel_format);
    const uint32_t   mip_levels_count            = GetMipLevelsCount();
    const uint32_t   required_subresources_count = GetRequiredSubresourceCount();

    if (!settings.mipmapped && sub_resources.size() < required_subresources_count)
    {
        throw std::invalid_argument("Number of sub-resources provided (" + std::to_string(sub_resources.size()) + 
                                    ") is less than required (" + std::to_string(required_subresources_count) + 
                                    ") for normal texture upload \"" + GetName() + "\".");
    }

    std::vector<D3D12_SUBRESOURCE_DATA> dx_sub_resources(required_subresources_count, D3D12_SUBRESOURCE_DATA{});
    for(const SubResource& sub_resource : sub_resources)
    {
        const uint32_t sub_resource_raw_index = sub_resource.GetRawIndex(settings.dimensions.depth, mip_levels_count);
        assert(sub_resource_raw_index < dx_sub_resources.size());
        if (sub_resource_raw_index >= dx_sub_resources.size())
            continue;

        D3D12_SUBRESOURCE_DATA& dx_sub_resource = dx_sub_resources[sub_resource_raw_index];
        dx_sub_resource.pData      = sub_resource.p_data;
        dx_sub_resource.RowPitch   = settings.dimensions.width  * pixel_size;
        dx_sub_resource.SlicePitch = settings.dimensions.height * dx_sub_resource.RowPitch;

        if (dx_sub_resource.SlicePitch > static_cast<LONG_PTR>(sub_resource.data_size))
        {
            throw std::invalid_argument("Sub-resource data size is smaller than computed slice size. Possible pixel format mismatch.");
        }

        m_data_size += static_cast<Data::Size>(dx_sub_resource.SlicePitch);
    }

    // NOTE: scratch_image is the owner of generated mip-levels memory, which should be hold until UpdateSubresources call completes
    DirectX::ScratchImage scratch_image; 
    if (settings.mipmapped && sub_resources.size() < required_subresources_count)
    {
        GenerateMipLevels(dx_sub_resources, scratch_image);
    }

    BlitCommandListDX& upload_cmd_list = static_cast<BlitCommandListDX&>(GetContext().GetUploadCommandList());
    UpdateSubresources(&upload_cmd_list.GetNativeCommandList(),
                       GetNativeResource(), m_cp_upload_resource.Get(), 0, 0,
                       static_cast<UINT>(dx_sub_resources.size()), dx_sub_resources.data());

    upload_cmd_list.SetResourceTransitionBarriers({ static_cast<Resource&>(*this) }, ResourceBase::State::CopyDest, ResourceBase::State::PixelShaderResource);
}

void ImageTextureDX::GenerateMipLevels(std::vector<D3D12_SUBRESOURCE_DATA>& dx_sub_resources, DirectX::ScratchImage& scratch_image)
{
    ITT_FUNCTION_TASK();

    const Settings&           settings          = GetSettings();
    const uint32_t            mip_levels_count  = GetMipLevelsCount();
    const D3D12_RESOURCE_DESC tex_desc          = GetNativeResourceRef().GetDesc();
    const bool                is_cube_texture   = settings.dimension_type == DimensionType::Cube || settings.dimension_type == DimensionType::CubeArray;

    std::vector<DirectX::Image> sub_resource_images(dx_sub_resources.size(), DirectX::Image{});
    for(uint32_t sub_resource_raw_index = 0; sub_resource_raw_index < dx_sub_resources.size(); ++sub_resource_raw_index)
    {
        // Initialize images of base mip-levels only
        const SubResource::Index sub_resource_index = SubResource::ComputeIndex(sub_resource_raw_index);
        if (sub_resource_index.mip_level > 0)
            continue;

        D3D12_SUBRESOURCE_DATA& dx_sub_resource = dx_sub_resources[sub_resource_raw_index];
        DirectX::Image& base_mip_image = sub_resource_images[sub_resource_raw_index];
        base_mip_image.width           = settings.dimensions.width;
        base_mip_image.height          = settings.dimensions.height;
        base_mip_image.format          = tex_desc.Format;
        base_mip_image.rowPitch        = dx_sub_resource.RowPitch;
        base_mip_image.slicePitch      = dx_sub_resource.SlicePitch;
        base_mip_image.pixels          = reinterpret_cast<uint8_t*>(const_cast<void*>(dx_sub_resource.pData)); // FIXME: Dirty casting...
    }

    DirectX::TexMetadata tex_metadata = { };
    tex_metadata.width     = settings.dimensions.width;
    tex_metadata.height    = settings.dimensions.height;
    tex_metadata.depth     = is_cube_texture ? 1 : settings.dimensions.depth;
    tex_metadata.arraySize = is_cube_texture ? settings.dimensions.depth : settings.array_length;
    tex_metadata.mipLevels = mip_levels_count;
    tex_metadata.format    = tex_desc.Format;
    tex_metadata.dimension = static_cast<DirectX::TEX_DIMENSION>(tex_desc.Dimension);
    tex_metadata.miscFlags = is_cube_texture ? DirectX::TEX_MISC_TEXTURECUBE : 0;

    ThrowIfFailed(DirectX::GenerateMipMaps(sub_resource_images.data(), sub_resource_images.size(),
                                           tex_metadata, DirectX::TEX_FILTER_DEFAULT,
                                           mip_levels_count, scratch_image));

    for (uint32_t depth = 0; depth < tex_metadata.depth; ++depth)
    {
        for (uint32_t item = 0; item < tex_metadata.arraySize; ++item)
        {
            for (uint32_t mip = 1; mip < tex_metadata.mipLevels; ++mip)
            {
                const DirectX::Image* p_mip_image = scratch_image.GetImage(mip, item, depth);
                if (!p_mip_image)
                {
                    throw std::runtime_error("Failed to generate mipmap level " + std::to_string(mip) +
                                             " for array item " + std::to_string(item) +
                                             " in depth " + std::to_string(depth) +
                                             " of texture \"" + GetName() + "\".");
                }

                const uint32_t dx_sub_resource_index = SubResource::ComputeRawIndex({ depth, item, mip }, static_cast<uint32_t>(tex_metadata.depth), static_cast<uint32_t>(tex_metadata.mipLevels));
                assert(dx_sub_resource_index < dx_sub_resources.size());

                D3D12_SUBRESOURCE_DATA& dx_sub_resource = dx_sub_resources[dx_sub_resource_index];
                dx_sub_resource.pData       = p_mip_image->pixels;
                dx_sub_resource.RowPitch    = p_mip_image->rowPitch;
                dx_sub_resource.SlicePitch  = p_mip_image->slicePitch;

                m_data_size += static_cast<Data::Size>(dx_sub_resource.SlicePitch);
            }
        }
    }
}

} // namespace Methane::Graphics
