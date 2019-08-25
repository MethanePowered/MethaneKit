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

FILE: Methane/Graphics/DirectX12/TextureDX.cpp
DirectX 12 implementation of the texture interface.

******************************************************************************/

#include "TextureDX.h"
#include "ContextDX.h"
#include "DeviceDX.h"
#include "DescriptorHeapDX.h"
#include "CommandQueueDX.h"
#include "RenderCommandListDX.h"
#include "TypesDX.h"

#include <Methane/Graphics/Instrumentation.h>
#include <Methane/Graphics/Windows/Helpers.h>

using namespace Methane;
using namespace Methane::Graphics;

D3D12_SRV_DIMENSION GetSrvDimension(const Dimensions& tex_dimensions) noexcept
{
    ITT_FUNCTION_TASK();
    return tex_dimensions.depth == 1 ? (tex_dimensions.height == 1 ? D3D12_SRV_DIMENSION_TEXTURE1D
                                                                   : D3D12_SRV_DIMENSION_TEXTURE2D)
                                     : D3D12_SRV_DIMENSION_TEXTURE3D;
}

D3D12_DSV_DIMENSION GetDsvDimension(const Dimensions& tex_dimensions)
{
    ITT_FUNCTION_TASK();
    if (tex_dimensions.depth != 1)
    {
        throw std::runtime_error("Depth-stencil view can not be created for 3D texture.");
    }
    return  tex_dimensions.height == 1 ? D3D12_DSV_DIMENSION_TEXTURE1D
                                       : D3D12_DSV_DIMENSION_TEXTURE2D;
}

Texture::Ptr Texture::CreateRenderTarget(Context& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
    
    switch (settings.type)
    {
    case Texture::Type::FrameBuffer:        throw std::logic_error("Frame buffer texture must be created with static method Texture::CreateFrameBuffer.");
    case Texture::Type::DepthStencilBuffer: return std::make_shared<DepthStencilBufferTextureDX>(static_cast<ContextBase&>(context), settings, descriptor_by_usage, context.GetSettings().clear_depth, context.GetSettings().clear_stencil);
    default:                                return std::make_shared<RenderTargetTextureDX>(static_cast<ContextBase&>(context), settings, descriptor_by_usage);
    }
}

Texture::Ptr Texture::CreateFrameBuffer(Context& context, uint32_t frame_buffer_index, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
    
    const Context::Settings& context_settings = context.GetSettings();
    const Settings texture_settings = Settings::FrameBuffer(context_settings.frame_size, context_settings.color_format);
    return std::make_shared<FrameBufferTextureDX>(static_cast<ContextBase&>(context), texture_settings, descriptor_by_usage, frame_buffer_index);
}

Texture::Ptr Texture::CreateDepthStencilBuffer(Context& context, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
    
    const Context::Settings& context_settings = context.GetSettings();
    const Settings texture_settings = Settings::DepthStencilBuffer(context_settings.frame_size, context_settings.depth_stencil_format);
    return std::make_shared<DepthStencilBufferTextureDX>(static_cast<ContextBase&>(context), texture_settings, descriptor_by_usage, context_settings.clear_depth, context_settings.clear_stencil);
}

Texture::Ptr Texture::CreateImage(Context& context, Dimensions dimensions, PixelFormat pixel_format, bool mipmapped, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();

    const Settings texture_settings = Settings::Image(dimensions, pixel_format, mipmapped, static_cast<Usage::Mask>(Usage::ShaderRead));
    return std::make_shared<ImageTextureDX>(static_cast<ContextBase&>(context), texture_settings, descriptor_by_usage, ImageTextureArg());
}

template<>
void RenderTargetTextureDX::Initialize()
{
    ITT_FUNCTION_TASK();

    if (m_settings.dimensions.depth != 1 || m_settings.dimensions.width == 0 || m_settings.dimensions.height == 0)
    {
        throw std::invalid_argument("Render target texture can only be created for 2D texture with dimensions.depth == 1 and non zero width and hight.");
    }

    D3D12_RESOURCE_DESC tex_desc = CD3DX12_RESOURCE_DESC::Tex2D(
        TypeConverterDX::DataFormatToDXGI(m_settings.pixel_format),
        m_settings.dimensions.width, m_settings.dimensions.height
    );
    InitializeCommittedResource(tex_desc, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_GENERIC_READ);
}

template<>
void FrameBufferTextureDX::Initialize(uint32_t frame_buffer_index)
{
    ITT_FUNCTION_TASK();

    InitializeFrameBufferResource(frame_buffer_index);

    if (m_usage_mask != Usage::RenderTarget)
    {
        throw std::runtime_error("Frame BufferBase texture supports only Render Target usage.");
    }

    assert(!!m_cp_resource);
    const D3D12_CPU_DESCRIPTOR_HANDLE descriptor_handle = GetNativeCPUDescriptorHandle(Usage::RenderTarget);
    GetContextDX().GetDeviceDX().GetNativeDevice()->CreateRenderTargetView(m_cp_resource.Get(), nullptr, descriptor_handle);
}

template<>
void DepthStencilBufferTextureDX::Initialize(Depth depth_clear_value, Stencil stencil_clear_value)
{
    ITT_FUNCTION_TASK();

    CD3DX12_RESOURCE_DESC tex_desc = tex_desc = CD3DX12_RESOURCE_DESC::Tex2D(
        TypeConverterDX::DataFormatToDXGI(m_settings.pixel_format, TypeConverterDX::ResourceFormatType::ResourceBase),
        m_settings.dimensions.width, m_settings.dimensions.height
    );

    tex_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    if (m_settings.usage_mask & Usage::RenderTarget)
    {
        tex_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }
    if (!(m_settings.usage_mask & Usage::ShaderRead ||
          m_settings.usage_mask & Usage::ShaderWrite))
    {
        tex_desc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    }

    // Performance tip: Tell the runtime at resource creation the desired clear value
    const DXGI_FORMAT view_write_format = TypeConverterDX::DataFormatToDXGI(m_settings.pixel_format, TypeConverterDX::ResourceFormatType::ViewWrite);
    CD3DX12_CLEAR_VALUE clear_value(view_write_format, depth_clear_value, stencil_clear_value);
    InitializeCommittedResource(tex_desc, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear_value);

    const wrl::ComPtr<ID3D12Device>& cp_device = GetContextDX().GetDeviceDX().GetNativeDevice();

    for (Usage::Value usage : Usage::values)
    {
        if (!(m_usage_mask & usage))
            continue;

        const Descriptor& desc = ResourceBase::GetDescriptorByUsage(usage);
        switch (desc.heap.GetSettings().type)
        {
        case DescriptorHeap::Type::ShaderResources:
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
            srv_desc.Format                          = TypeConverterDX::DataFormatToDXGI(m_settings.pixel_format, TypeConverterDX::ResourceFormatType::ViewRead);
            srv_desc.ViewDimension                   = GetSrvDimension(m_settings.dimensions);
            srv_desc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srv_desc.Texture2D.MipLevels             = 1;
            cp_device->CreateShaderResourceView(m_cp_resource.Get(), &srv_desc, GetNativeCPUDescriptorHandle(desc));
        } break;
        case DescriptorHeap::Type::DepthStencil:
        {
            D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc   = {};
            dsv_desc.Format                          = view_write_format;
            dsv_desc.ViewDimension                   = GetDsvDimension(m_settings.dimensions);
            cp_device->CreateDepthStencilView(m_cp_resource.Get(), &dsv_desc, GetNativeCPUDescriptorHandle(desc));
        } break;
        default:
            throw std::runtime_error("Unsupported usage of Depth-Stencil buffer.");
        }
    }
}

ImageTextureDX::TextureDX(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage, ImageTextureArg)
    : TextureBase(context, settings, descriptor_by_usage)
{
    ITT_FUNCTION_TASK();

    if (m_usage_mask != Usage::ShaderRead)
    {
        throw std::runtime_error("Image texture supports only \"Shader Read\" usage.");
    }

    InitializeDefaultDescriptors();

    assert(m_settings.dimensions.depth > 0);
    assert(m_settings.dimensions.width > 0);
    assert(m_settings.dimensions.height > 0);

    D3D12_RESOURCE_DESC tex_desc = {};
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};

    if (m_settings.dimensions.depth == 1)
    {
        if (m_settings.dimensions.height == 1)
        {
            tex_desc = CD3DX12_RESOURCE_DESC::Tex1D(
                TypeConverterDX::DataFormatToDXGI(m_settings.pixel_format),
                m_settings.dimensions.width
            );
            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
            srv_desc.Texture1D.MipLevels = 1;
        }
        else
        {
            tex_desc = CD3DX12_RESOURCE_DESC::Tex2D(
                TypeConverterDX::DataFormatToDXGI(m_settings.pixel_format),
                m_settings.dimensions.width, m_settings.dimensions.height,
                1, m_settings.mipmapped ? 0 : 1
            );
            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srv_desc.Texture2D.MipLevels = 1;
        }
    }
    else
    {
        tex_desc = CD3DX12_RESOURCE_DESC::Tex3D(
            TypeConverterDX::DataFormatToDXGI(m_settings.pixel_format),
            m_settings.dimensions.width, m_settings.dimensions.height, m_settings.dimensions.depth
        );
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        srv_desc.Texture3D.MipLevels = 1;
    }

    srv_desc.Format = tex_desc.Format;
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    InitializeCommittedResource(tex_desc, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COPY_DEST);

    const UINT64 upload_buffer_size = GetRequiredIntermediateSize(m_cp_resource.Get(), 0, 1);
    ThrowIfFailed(
        GetContextDX().GetDeviceDX().GetNativeDevice()->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(upload_buffer_size),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_cp_upload_resource)
        )
    );

    GetContextDX().GetDeviceDX().GetNativeDevice()->CreateShaderResourceView(m_cp_resource.Get(), &srv_desc, GetNativeCPUDescriptorHandle(Usage::ShaderRead));
}

void ImageTextureDX::SetData(Data::ConstRawPtr p_data, Data::Size data_size)
{
    ITT_FUNCTION_TASK();

    if (!p_data || data_size == 0)
    {
        throw std::invalid_argument("Can not set empty data to texture.");
    }
    
    assert(!!m_cp_resource);
    assert(!!m_cp_upload_resource);

    D3D12_SUBRESOURCE_DATA texture_data = {};
    texture_data.pData      = p_data;
    texture_data.RowPitch   = m_settings.dimensions.width * GetPixelSize(m_settings.pixel_format);
    texture_data.SlicePitch = m_settings.dimensions.height * texture_data.RowPitch;

    assert(texture_data.SlicePitch <= data_size);
    m_data_size = data_size;

    RenderCommandListDX& upload_cmd_list = static_cast<RenderCommandListDX&>(m_context.GetUploadCommandList());
    UpdateSubresources(upload_cmd_list.GetNativeCommandList().Get(),
                       m_cp_resource.Get(), m_cp_upload_resource.Get(), 0, 0, 1, &texture_data);

    upload_cmd_list.SetResourceTransitionBarriers({ static_cast<Resource&>(*this) }, ResourceBase::State::CopyDest, ResourceBase::State::PixelShaderResource);
}