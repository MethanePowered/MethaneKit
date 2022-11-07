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

FILE: Methane/Graphics/DirectX/Texture.cpp
DirectX 12 implementation of the texture interface.

******************************************************************************/

#include <Methane/Graphics/DirectX/Texture.h>
#include <Methane/Graphics/DirectX/RenderContext.h>
#include <Methane/Graphics/DirectX/Device.h>
#include <Methane/Graphics/DirectX/DescriptorHeap.h>
#include <Methane/Graphics/DirectX/CommandQueue.h>
#include <Methane/Graphics/DirectX/TransferCommandList.h>
#include <Methane/Graphics/DirectX/Types.h>

#include <Methane/Graphics/Windows/DirectXErrorHandling.h>
#include <Methane/Graphics/TypeFormatters.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <fmt/format.h>
#include <magic_enum.hpp>
#include <DirectXTex.h>

namespace Methane::Graphics
{

Ptr<ITexture> ITexture::CreateRenderTarget(const IRenderContext& render_context, const Settings& settings)
{
    META_FUNCTION_TASK();
    switch (settings.type)
    {
    case ITexture::Type::Texture:            return std::make_shared<DirectX::RenderTargetTexture>(static_cast<const Base::RenderContext&>(render_context), settings);
    case ITexture::Type::DepthStencilBuffer: return std::make_shared<DirectX::DepthStencilTexture>(static_cast<const Base::RenderContext&>(render_context), settings, render_context.GetSettings().clear_depth_stencil);
    case ITexture::Type::FrameBuffer:        META_UNEXPECTED_ARG_DESCR(settings.type, "frame buffer texture must be created with static method Texture::CreateFrameBuffer");
    default:                                 META_UNEXPECTED_ARG_RETURN(settings.type, nullptr);
    }
}

Ptr<ITexture> ITexture::CreateFrameBuffer(const IRenderContext& render_context, FrameBufferIndex frame_buffer_index)
{
    META_FUNCTION_TASK();
    const RenderContextSettings& context_settings = render_context.GetSettings();
    const Settings texture_settings = Settings::FrameBuffer(Dimensions(context_settings.frame_size), context_settings.color_format);
    return std::make_shared<DirectX::FrameBufferTexture>(static_cast<const Base::RenderContext&>(render_context), texture_settings, frame_buffer_index);
}

Ptr<ITexture> ITexture::CreateDepthStencilBuffer(const IRenderContext& render_context)
{
    META_FUNCTION_TASK();
    const RenderContextSettings& context_settings = render_context.GetSettings();
    const Settings texture_settings = Settings::DepthStencilBuffer(Dimensions(context_settings.frame_size), context_settings.depth_stencil_format);
    return std::make_shared<DirectX::DepthStencilTexture>(static_cast<const Base::RenderContext&>(render_context), texture_settings, context_settings.clear_depth_stencil);
}

Ptr<ITexture> ITexture::CreateImage(const IContext& render_context, const Dimensions& dimensions, const Opt<uint32_t>& array_length_opt, PixelFormat pixel_format, bool mipmapped)
{
    META_FUNCTION_TASK();
    const Settings texture_settings = Settings::Image(dimensions, array_length_opt, pixel_format, mipmapped, Usage::ShaderRead);
    return std::make_shared<DirectX::ImageTexture>(dynamic_cast<const Base::Context&>(render_context), texture_settings, DirectX::ImageToken());
}

Ptr<ITexture> ITexture::CreateCube(const IContext& render_context, uint32_t dimension_size, const Opt<uint32_t>& array_length_opt, PixelFormat pixel_format, bool mipmapped)
{
    META_FUNCTION_TASK();
    const Settings texture_settings = Settings::Cube(dimension_size, array_length_opt, pixel_format, mipmapped, Usage::ShaderRead);
    return std::make_shared<DirectX::ImageTexture>(dynamic_cast<const Base::Context&>(render_context), texture_settings, DirectX::ImageToken());
}

} // namespace Methane::Graphics

namespace Methane::Graphics::DirectX
{

[[nodiscard]]
static D3D12_SRV_DIMENSION GetSrvDimension(const Dimensions& tex_dimensions) noexcept
{
    META_FUNCTION_TASK();
    const D3D12_SRV_DIMENSION flat_dimension = tex_dimensions.GetHeight() == 1 ? D3D12_SRV_DIMENSION_TEXTURE1D : D3D12_SRV_DIMENSION_TEXTURE2D;
    return tex_dimensions.GetDepth() == 1 ? flat_dimension : D3D12_SRV_DIMENSION_TEXTURE3D;
}

[[nodiscard]]
static D3D12_DSV_DIMENSION GetDsvDimension(const Dimensions& tex_dimensions)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL_DESCR(tex_dimensions.GetDepth(), 1, "depth-stencil view can not be created for 3D texture");
    return  tex_dimensions.GetHeight() == 1 ? D3D12_DSV_DIMENSION_TEXTURE1D
                                            : D3D12_DSV_DIMENSION_TEXTURE2D;
}

[[nodiscard]]
static CD3DX12_RESOURCE_DESC CreateNativeResourceDesc(const ITexture::Settings& settings, const SubResource::Count& sub_resource_count)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_GREATER_OR_EQUAL(settings.dimensions.GetDepth(), 1);
    META_CHECK_ARG_GREATER_OR_EQUAL(settings.dimensions.GetWidth(), 1);
    META_CHECK_ARG_GREATER_OR_EQUAL(settings.dimensions.GetHeight(), 1);

    CD3DX12_RESOURCE_DESC tex_desc{};
    switch (settings.dimension_type)
    {
    case TextureDimensionType::Tex1D:
        META_CHECK_ARG_EQUAL_DESCR(settings.array_length, 1, "single 1D texture must have array length equal to 1");
        [[fallthrough]];

    case TextureDimensionType::Tex1DArray:
        META_CHECK_ARG_DESCR(settings.dimensions, settings.dimensions.GetHeight() == 1 && settings.dimensions.GetDepth() == 1,
                             "1D textures must have height and depth dimensions equal to 1");
        tex_desc = CD3DX12_RESOURCE_DESC::Tex1D(
            TypeConverter::PixelFormatToDxgi(settings.pixel_format),
            settings.dimensions.GetWidth(),
            static_cast<UINT16>(sub_resource_count.GetArraySize()),
            static_cast<UINT16>(sub_resource_count.GetMipLevelsCount())
        );
        break;

    case TextureDimensionType::Tex2DMultisample:
        META_UNEXPECTED_ARG_DESCR(settings.dimension_type, "2D Multisample textures are not supported yet");

    case TextureDimensionType::Tex2D:
        META_CHECK_ARG_EQUAL_DESCR(settings.array_length, 1, "single 2D texture must have array length equal to 1");
        [[fallthrough]];

    case TextureDimensionType::Tex2DArray:
        META_CHECK_ARG_EQUAL_DESCR(settings.dimensions.GetDepth(), 1, "2D textures must have depth dimension equal to 1");
        tex_desc = CD3DX12_RESOURCE_DESC::Tex2D(
            TypeConverter::PixelFormatToDxgi(settings.pixel_format),
            settings.dimensions.GetWidth(),
            settings.dimensions.GetHeight(),
            static_cast<UINT16>(sub_resource_count.GetArraySize()),
            static_cast<UINT16>(sub_resource_count.GetMipLevelsCount())
        );
        break;

    case TextureDimensionType::Tex3D:
        META_CHECK_ARG_EQUAL_DESCR(settings.array_length, 1, "single 3D texture must have array length equal to 1");
        tex_desc = CD3DX12_RESOURCE_DESC::Tex3D(
            TypeConverter::PixelFormatToDxgi(settings.pixel_format),
            settings.dimensions.GetWidth(),
            settings.dimensions.GetHeight(),
            static_cast<UINT16>(sub_resource_count.GetDepth()),
            static_cast<UINT16>(sub_resource_count.GetMipLevelsCount())
        );
        break;

    case TextureDimensionType::Cube:
        META_CHECK_ARG_EQUAL_DESCR(settings.array_length, 1, "single Cube texture must have array length equal to 1");
        [[fallthrough]];

    case TextureDimensionType::CubeArray:
        META_CHECK_ARG_EQUAL_DESCR(settings.dimensions.GetDepth(), 6, "Cube textures depth dimension must be equal to 6");
        tex_desc = CD3DX12_RESOURCE_DESC::Tex2D(
            TypeConverter::PixelFormatToDxgi(settings.pixel_format),
            settings.dimensions.GetWidth(),
            settings.dimensions.GetHeight(),
            static_cast<UINT16>(sub_resource_count.GetDepth() * sub_resource_count.GetArraySize()),
            static_cast<UINT16>(sub_resource_count.GetMipLevelsCount())
        );
        break;

    default:
        META_UNEXPECTED_ARG(settings.dimension_type);
    }

    return tex_desc;
}

[[nodiscard]]
static D3D12_SHADER_RESOURCE_VIEW_DESC CreateNativeShaderResourceViewDesc(const ITexture::Settings& settings, const ResourceView::Id& view_id)
{
    META_FUNCTION_TASK();
    const IResource::SubResource::Index& sub_resource_index = view_id.subresource_index;
    const IResource::SubResource::Count& sub_resource_count = view_id.subresource_count;

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
    switch (settings.dimension_type)
    {
    case TextureDimensionType::Tex1D:
        srv_desc.Texture1D.MostDetailedMip      = sub_resource_index.GetMipLevel();
        srv_desc.Texture1D.MipLevels            = sub_resource_count.GetMipLevelsCount();
        srv_desc.ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURE1D;
        break;

    case TextureDimensionType::Tex1DArray:
        srv_desc.Texture1DArray.MostDetailedMip = sub_resource_index.GetMipLevel();
        srv_desc.Texture1DArray.MipLevels       = sub_resource_count.GetMipLevelsCount();
        srv_desc.Texture1DArray.FirstArraySlice = sub_resource_index.GetArrayIndex();
        srv_desc.Texture1DArray.ArraySize       = sub_resource_count.GetArraySize();
        srv_desc.ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
        break;

    case TextureDimensionType::Tex2DMultisample:
    case TextureDimensionType::Tex2D:
        srv_desc.Texture2D.MostDetailedMip      = sub_resource_index.GetMipLevel();
        srv_desc.Texture2D.MipLevels            = sub_resource_count.GetMipLevelsCount();
        srv_desc.ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURE2D;
        break;

    case TextureDimensionType::Tex2DArray:
        srv_desc.Texture2DArray.MostDetailedMip = sub_resource_index.GetMipLevel();
        srv_desc.Texture2DArray.MipLevels       = sub_resource_count.GetMipLevelsCount();
        srv_desc.Texture2DArray.FirstArraySlice = sub_resource_index.GetArrayIndex();
        srv_desc.Texture2DArray.ArraySize       = sub_resource_count.GetArraySize();
        srv_desc.ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        break;

    case TextureDimensionType::Tex3D:
        srv_desc.Texture3D.MostDetailedMip      = sub_resource_index.GetMipLevel();
        srv_desc.Texture3D.MipLevels            = sub_resource_count.GetMipLevelsCount();
        srv_desc.ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURE3D;
        break;

    case TextureDimensionType::Cube:
        srv_desc.TextureCube.MostDetailedMip    = sub_resource_index.GetMipLevel();
        srv_desc.TextureCube.MipLevels          = sub_resource_count.GetMipLevelsCount();
        srv_desc.ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURECUBE;
        break;

    case TextureDimensionType::CubeArray:
        srv_desc.TextureCubeArray.First2DArrayFace = sub_resource_index.GetArrayIndex() * 6U + sub_resource_index.GetDepthSlice();
        srv_desc.TextureCubeArray.NumCubes         = sub_resource_count.GetArraySize();
        srv_desc.TextureCubeArray.MostDetailedMip  = sub_resource_index.GetMipLevel();
        srv_desc.TextureCubeArray.MipLevels        = sub_resource_count.GetMipLevelsCount();
        srv_desc.ViewDimension                     = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        break;

    default:
        META_UNEXPECTED_ARG(settings.dimension_type);
    }

    srv_desc.Format                  = TypeConverter::PixelFormatToDxgi(settings.pixel_format);
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    return srv_desc;
}

[[nodiscard]]
static D3D12_RENDER_TARGET_VIEW_DESC CreateNativeRenderTargetViewDesc(const ITexture::Settings& settings,
                                                                      const ResourceView::Id& view_id)
{
    META_FUNCTION_TASK();
    const IResource::SubResource::Index& sub_resource_index = view_id.subresource_index;
    const IResource::SubResource::Count& sub_resource_count = view_id.subresource_count;

    D3D12_RENDER_TARGET_VIEW_DESC rtv_desc{};
    switch (settings.dimension_type)
    {
    case TextureDimensionType::Tex1D:
        rtv_desc.Texture1D.MipSlice             = sub_resource_index.GetMipLevel();
        rtv_desc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE1D;
        break;

    case TextureDimensionType::Tex1DArray:
        rtv_desc.Texture1DArray.MipSlice        = sub_resource_index.GetMipLevel();
        rtv_desc.Texture1DArray.FirstArraySlice = sub_resource_index.GetArrayIndex();
        rtv_desc.Texture1DArray.ArraySize       = sub_resource_count.GetArraySize();
        rtv_desc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
        break;

    case TextureDimensionType::Tex2DMultisample:
    case TextureDimensionType::Tex2D:
        rtv_desc.Texture2D.MipSlice             = sub_resource_index.GetMipLevel();
        rtv_desc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2D;
        break;

    case TextureDimensionType::Cube:
    case TextureDimensionType::CubeArray:
    case TextureDimensionType::Tex2DArray:
        rtv_desc.Texture2DArray.MipSlice        = sub_resource_index.GetMipLevel();
        rtv_desc.Texture2DArray.FirstArraySlice = settings.dimension_type == TextureDimensionType::Tex2DArray
                                                ? sub_resource_index.GetArrayIndex()
                                                : (sub_resource_index.GetArrayIndex() * 6U + sub_resource_index.GetDepthSlice());
        rtv_desc.Texture2DArray.ArraySize       = sub_resource_count.GetArraySize();
        rtv_desc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        break;

    case TextureDimensionType::Tex3D:
        rtv_desc.Texture3D.MipSlice             = sub_resource_index.GetMipLevel();
        rtv_desc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE3D;
        break;

    default:
        META_UNEXPECTED_ARG(settings.dimension_type);
    }

    rtv_desc.Format                  = TypeConverter::PixelFormatToDxgi(settings.pixel_format);
    return rtv_desc;
}

template<>
void FrameBufferTexture::Initialize(FrameBufferIndex frame_buffer_index)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL_DESCR(GetUsage(), Usage::RenderTarget, "frame-buffer texture supports only 'RenderTarget' usage");
    InitializeFrameBufferResource(frame_buffer_index);
}

template<>
Opt<IResource::Descriptor> FrameBufferTexture::InitializeNativeViewDescriptor(const View::Id& view_id)
{
    META_FUNCTION_TASK();
    const IResource::Descriptor& descriptor = GetDescriptorByViewId(view_id);
    GetDirectContext().GetDirectDevice().GetNativeDevice()->CreateRenderTargetView(GetNativeResource(), nullptr, GetNativeCpuDescriptorHandle(descriptor));
    return descriptor;
}

RenderTargetTexture::Texture(const Base::Context& context, const Settings& settings)
    : Resource(context, settings)
{
    META_FUNCTION_TASK();
    D3D12_RESOURCE_DESC tex_desc = CreateNativeResourceDesc(GetSettings(), GetSubresourceCount());
    tex_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    InitializeCommittedResource(tex_desc, D3D12_HEAP_TYPE_DEFAULT, IResource::State::RenderTarget);
}

Opt<IResource::Descriptor> RenderTargetTexture::InitializeNativeViewDescriptor(const View::Id& view_id)
{
    META_FUNCTION_TASK();
    const IResource::Descriptor& descriptor = GetDescriptorByViewId(view_id);
    switch(view_id.usage)
    {
    case ResourceUsage::ShaderRead:   CreateShaderResourceView(descriptor, view_id); break;
    case ResourceUsage::RenderTarget: CreateRenderTargetView(descriptor, view_id); break;
    default: break;
    }
    return descriptor;
}

void RenderTargetTexture::CreateShaderResourceView(const Descriptor& descriptor, const View::Id& view_id) const
{
    META_FUNCTION_TASK();
    const D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = CreateNativeShaderResourceViewDesc(GetSettings(), view_id);
    GetDirectContext().GetDirectDevice().GetNativeDevice()->CreateShaderResourceView(GetNativeResource(), &srv_desc, GetNativeCpuDescriptorHandle(descriptor));
}

void RenderTargetTexture::CreateRenderTargetView(const Descriptor& descriptor, const View::Id& view_id) const
{
    META_FUNCTION_TASK();
    const D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = CreateNativeRenderTargetViewDesc(GetSettings(), view_id);
    GetDirectContext().GetDirectDevice().GetNativeDevice()->CreateRenderTargetView(GetNativeResource(), &rtv_desc, GetNativeCpuDescriptorHandle(descriptor));
}

DepthStencilTexture::Texture(const Base::Context& render_context, const Settings& settings,
                                 const Opt<DepthStencil>& clear_depth_stencil)
    : Resource(render_context, settings)
{
    META_FUNCTION_TASK();

    CD3DX12_RESOURCE_DESC tex_desc = CD3DX12_RESOURCE_DESC::Tex2D(
        TypeConverter::PixelFormatToDxgi(settings.pixel_format, TypeConverter::ResourceFormatType::Resource),
        settings.dimensions.GetWidth(), settings.dimensions.GetHeight(),
        1, // array size
        1  // mip levels
    );

    using namespace magic_enum::bitwise_operators;
    if (static_cast<bool>(settings.usage_mask & Usage::RenderTarget))
    {
        tex_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }
    if (!static_cast<bool>(settings.usage_mask & (Usage::ShaderRead | Usage::ShaderWrite)))
    {
        tex_desc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    }

    if (clear_depth_stencil)
    {
        // Performance tip: Tell the runtime at resource creation the desired clear value
        const DXGI_FORMAT view_write_format = TypeConverter::PixelFormatToDxgi(settings.pixel_format, TypeConverter::ResourceFormatType::ViewWrite);
        CD3DX12_CLEAR_VALUE clear_value(view_write_format, clear_depth_stencil->first, clear_depth_stencil->second);
        InitializeCommittedResource(tex_desc, D3D12_HEAP_TYPE_DEFAULT, IResource::State::DepthWrite, &clear_value);
    }
    else
    {
        InitializeCommittedResource(tex_desc, D3D12_HEAP_TYPE_DEFAULT, IResource::State::DepthWrite);
    }
}

void DepthStencilTexture::CreateShaderResourceView(const Descriptor& descriptor) const
{
    META_FUNCTION_TASK();
    const ITexture::Settings& settings = GetSettings();

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
    srv_desc.Format                  = TypeConverter::PixelFormatToDxgi(settings.pixel_format, TypeConverter::ResourceFormatType::ViewRead);
    srv_desc.ViewDimension           = GetSrvDimension(settings.dimensions);
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.Texture2D.MipLevels     = 1;

    const D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor_handle = GetNativeCpuDescriptorHandle(descriptor);
    GetDirectContext().GetDirectDevice().GetNativeDevice()->CreateShaderResourceView(GetNativeResource(), &srv_desc, cpu_descriptor_handle);
}

void DepthStencilTexture::CreateDepthStencilView(const Descriptor& descriptor) const
{
    META_FUNCTION_TASK();
    const ITexture::Settings& settings = GetSettings();

    D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
    dsv_desc.Format        = TypeConverter::PixelFormatToDxgi(settings.pixel_format, TypeConverter::ResourceFormatType::ViewWrite);
    dsv_desc.ViewDimension = GetDsvDimension(settings.dimensions);
    dsv_desc.Texture2D.MipSlice = 0;

    const D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor_handle = GetNativeCpuDescriptorHandle(descriptor);
    GetDirectContext().GetDirectDevice().GetNativeDevice()->CreateDepthStencilView(GetNativeResource(), &dsv_desc, cpu_descriptor_handle);
}

Opt<IResource::Descriptor> DepthStencilTexture::InitializeNativeViewDescriptor(const View::Id& view_id)
{
    META_FUNCTION_TASK();
    const IResource::Descriptor& descriptor = GetDescriptorByViewId(view_id);
    switch(view_id.usage)
    {
    case IResource::Usage::ShaderRead:   CreateShaderResourceView(descriptor); break;
    case IResource::Usage::RenderTarget: CreateDepthStencilView(descriptor); break;
    default: META_UNEXPECTED_ARG_DESCR_RETURN(view_id.usage, descriptor,
                                              "unsupported usage '{}' for Depth-Stencil buffer",
                                              magic_enum::enum_name(view_id.usage));
    }
    return descriptor;
}

ImageTexture::Texture(const Base::Context& render_context, const Settings& settings, ImageToken)
    : Resource(render_context, settings)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL_DESCR(GetUsage(), Usage::ShaderRead, "image texture supports only 'ShaderRead' usage");

    const SubResource::Count& sub_resource_count = GetSubresourceCount();
    const CD3DX12_RESOURCE_DESC resource_desc = CreateNativeResourceDesc(settings, sub_resource_count);
    InitializeCommittedResource(resource_desc, D3D12_HEAP_TYPE_DEFAULT, IResource::State::CopyDest);

    const UINT64 upload_buffer_size = GetRequiredIntermediateSize(GetNativeResource(), 0, GetSubresourceCount().GetRawCount());
    m_cp_upload_resource = CreateCommittedResource(CD3DX12_RESOURCE_DESC::Buffer(upload_buffer_size), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
}

Opt<IResource::Descriptor> ImageTexture::InitializeNativeViewDescriptor(const View::Id& view_id)
{
    META_FUNCTION_TASK();
    const IResource::Descriptor& descriptor = GetDescriptorByViewId(view_id);
    const D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor_handle = GetNativeCpuDescriptorHandle(descriptor);
    const D3D12_SHADER_RESOURCE_VIEW_DESC rtv_desc = CreateNativeShaderResourceViewDesc(GetSettings(), view_id);
    GetDirectContext().GetDirectDevice().GetNativeDevice()->CreateShaderResourceView(GetNativeResource(), &rtv_desc, cpu_descriptor_handle);
    return descriptor;
}

bool ImageTexture::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (!Resource::SetName(name))
        return false;

    META_CHECK_ARG_NOT_NULL(m_cp_upload_resource);
    m_cp_upload_resource->SetName(nowide::widen(fmt::format("{} Upload Resource", name)).c_str());
    return true;
}

void ImageTexture::SetData(const SubResources& sub_resources, ICommandQueue& target_cmd_queue)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_cp_upload_resource);

    Resource::SetData(sub_resources, target_cmd_queue);

    const Settings&  settings                    = GetSettings();
    const Data::Size pixel_size                  = GetPixelSize(settings.pixel_format);
    const SubResource::Count& sub_resource_count = GetSubresourceCount();
    const uint32_t       sub_resources_raw_count = sub_resource_count.GetRawCount();

    std::vector<D3D12_SUBRESOURCE_DATA> dx_sub_resources(sub_resources_raw_count, D3D12_SUBRESOURCE_DATA{});
    for(const SubResource& sub_resource : sub_resources)
    {
        ValidateSubResource(sub_resource);

        const uint32_t sub_resource_raw_index = sub_resource.GetIndex().GetRawIndex(sub_resource_count);
        META_CHECK_ARG_LESS(sub_resource_raw_index, dx_sub_resources.size());

        D3D12_SUBRESOURCE_DATA& dx_sub_resource = dx_sub_resources[sub_resource_raw_index];
        dx_sub_resource.pData      = sub_resource.GetDataPtr();
        dx_sub_resource.RowPitch   = static_cast<int64_t>(settings.dimensions.GetWidth())  * pixel_size;
        dx_sub_resource.SlicePitch = dx_sub_resource.RowPitch * settings.dimensions.GetHeight();

        META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(sub_resource.GetDataSize(), dx_sub_resource.SlicePitch,
                                              "sub-resource data size is less than computed MIP slice size, possibly due to pixel format mismatch");
    }

    // NOTE: scratch_image is the owner of generated mip-levels memory, which should be hold until UpdateSubresources call completes
    ::DirectX::ScratchImage scratch_image;
    if (settings.mipmapped && sub_resources.size() < sub_resources_raw_count)
    {
        GenerateMipLevels(dx_sub_resources, scratch_image);
    }

    // Upload texture subresources data to GPU via intermediate upload resource
    const TransferCommandList& upload_cmd_list = PrepareResourceUpload(target_cmd_queue);
    UpdateSubresources(&upload_cmd_list.GetNativeCommandList(),
                       GetNativeResource(), m_cp_upload_resource.Get(), 0, 0,
                       static_cast<UINT>(dx_sub_resources.size()), dx_sub_resources.data());
    GetContext().RequestDeferredAction(IContext::DeferredAction::UploadResources);
}

void ImageTexture::GenerateMipLevels(std::vector<D3D12_SUBRESOURCE_DATA>& dx_sub_resources, ::DirectX::ScratchImage& scratch_image) const
{
    META_FUNCTION_TASK();

    const Settings&           settings           = GetSettings();
    const SubResource::Count& sub_resource_count = GetSubresourceCount();
    const D3D12_RESOURCE_DESC tex_desc           = GetNativeResourceRef().GetDesc();
    const bool                is_cube_texture    = settings.dimension_type == DimensionType::Cube || settings.dimension_type == DimensionType::CubeArray;

    std::vector<::DirectX::Image> sub_resource_images(dx_sub_resources.size(), ::DirectX::Image{});
    for(uint32_t sub_resource_raw_index = 0; sub_resource_raw_index < dx_sub_resources.size(); ++sub_resource_raw_index)
    {
        // Initialize images of base mip-levels only
        if (SubResource::Index(sub_resource_raw_index, sub_resource_count).GetMipLevel() > 0)
            continue;

        const D3D12_SUBRESOURCE_DATA& dx_sub_resource = dx_sub_resources[sub_resource_raw_index];
        ::DirectX::Image& base_mip_image = sub_resource_images[sub_resource_raw_index];
        base_mip_image.width             = settings.dimensions.GetWidth();
        base_mip_image.height            = settings.dimensions.GetHeight();
        base_mip_image.format            = tex_desc.Format;
        base_mip_image.rowPitch          = dx_sub_resource.RowPitch;
        base_mip_image.slicePitch        = dx_sub_resource.SlicePitch;
        base_mip_image.pixels            = reinterpret_cast<uint8_t*>(const_cast<void*>(dx_sub_resource.pData)); // NOSONAR
    }

    ::DirectX::TexMetadata tex_metadata{ };
    tex_metadata.width     = settings.dimensions.GetWidth();
    tex_metadata.height    = settings.dimensions.GetHeight();
    tex_metadata.depth     = is_cube_texture ? 1 : settings.dimensions.GetDepth();
    tex_metadata.arraySize = is_cube_texture ? settings.dimensions.GetDepth() : settings.array_length;
    tex_metadata.mipLevels = sub_resource_count.GetMipLevelsCount();
    tex_metadata.format    = tex_desc.Format;
    tex_metadata.dimension = static_cast<::DirectX::TEX_DIMENSION>(tex_desc.Dimension);
    tex_metadata.miscFlags = is_cube_texture ? ::DirectX::TEX_MISC_TEXTURECUBE : 0;

    const SubResource::Count tex_metadata_subres_count(
        static_cast<Data::Size>(tex_metadata.depth),
        static_cast<Data::Size>(tex_metadata.arraySize),
        static_cast<Data::Size>(tex_metadata.mipLevels)
    );

    ThrowIfFailed(::DirectX::GenerateMipMaps(sub_resource_images.data(), sub_resource_images.size(),
                                             tex_metadata, ::DirectX::TEX_FILTER_DEFAULT,
                                             sub_resource_count.GetMipLevelsCount(), scratch_image));

    for (uint32_t depth = 0; depth < tex_metadata.depth; ++depth)
    {
        for (uint32_t item = 0; item < tex_metadata.arraySize; ++item)
        {
            for (uint32_t mip = 1; mip < tex_metadata.mipLevels; ++mip)
            {
                const ::DirectX::Image* p_mip_image = scratch_image.GetImage(mip, item, depth);
                META_CHECK_ARG_NOT_NULL_DESCR(p_mip_image,
                                              "failed to generate mipmap level {} for array item {} in depth {} of texture '{}'",
                                              mip, item, depth, GetName());

                const uint32_t dx_sub_resource_index = SubResource::Index(depth, item, mip).GetRawIndex(tex_metadata_subres_count);
                META_CHECK_ARG_LESS(dx_sub_resource_index, dx_sub_resources.size());

                D3D12_SUBRESOURCE_DATA& dx_sub_resource = dx_sub_resources[dx_sub_resource_index];
                dx_sub_resource.pData       = p_mip_image->pixels;
                dx_sub_resource.RowPitch    = p_mip_image->rowPitch;
                dx_sub_resource.SlicePitch  = p_mip_image->slicePitch;
            }
        }
    }
}

} // namespace Methane::Graphics::DirectX
