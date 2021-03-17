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

FILE: Methane/Graphics/DirectX12/TextureDX.h
DirectX 12 implementation of the texture interface.

******************************************************************************/

#pragma once

#include "ResourceDX.hpp"

#include <Methane/Graphics/TextureBase.h>
#include <Methane/Graphics/CommandListBase.h>
#include <Methane/Graphics/Types.h>
#include <Methane/Graphics/Windows/ErrorHandling.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <d3dx12.h>
#include <optional>

namespace DirectX
{
class ScratchImage;
}

namespace Methane::Graphics
{

class ContextBase;

template<typename... ExtraArgs>
class TextureDX final : public ResourceDX<TextureBase>
{
public:
    TextureDX(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage, ExtraArgs... extra_args)
        : ResourceDX<TextureBase>(context, settings, descriptor_by_usage)
    {
        META_FUNCTION_TASK();
        InitializeDefaultDescriptors();
        Initialize(extra_args...);
    }

    // Resource interface
    void SetData(const SubResources&) override
    {
        META_FUNCTION_NOT_IMPLEMENTED_DESCR("setting texture data is allowed for image textures only");
    }

private:
    void Initialize(ExtraArgs...);
};

struct ImageTextureArg { };

template<>
class TextureDX<ImageTextureArg> final : public ResourceDX<TextureBase>
{
public:
    TextureDX(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage, ImageTextureArg);

    // Object overrides
    void SetName(const std::string& name) override;

    // Resource overrides
    void SetData(const SubResources& sub_resources) override;

private:
    using ResourceAndViewDesc = std::pair<D3D12_RESOURCE_DESC, D3D12_SHADER_RESOURCE_VIEW_DESC>;
    ResourceAndViewDesc GetResourceAndViewDesc() const;
    void GenerateMipLevels(std::vector<D3D12_SUBRESOURCE_DATA>& dx_sub_resources, DirectX::ScratchImage& scratch_image) const;

    wrl::ComPtr<ID3D12Resource> m_cp_upload_resource;
    Ptr<ResourceBase::Barriers> m_upload_begin_transition_barriers_ptr;
};

template<>
class TextureDX<const std::optional<DepthStencil>&> final : public ResourceDX<TextureBase>
{
public:
    TextureDX(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage, const std::optional<DepthStencil>& clear_depth_stencil);

    // Resource overrides
    void SetData(const SubResources&) override
    {
        META_FUNCTION_NOT_IMPLEMENTED_DESCR("depth stencil texture does not allow to set data");
    }

private:
    void CreateShaderResourceView(const Texture::Settings& settings, const wrl::ComPtr<ID3D12Device>& cp_device, const Resource::Descriptor& desc) const;
    void CreateDepthStencilView(const Settings& settings, const DXGI_FORMAT& view_write_format, const wrl::ComPtr<ID3D12Device>& cp_device, const Resource::Descriptor& desc) const;
};

using RenderTargetTextureDX         = TextureDX<>;
using FrameBufferTextureDX          = TextureDX<uint32_t /* frame_buffer_index */>;
using DepthStencilBufferTextureDX   = TextureDX<const std::optional<DepthStencil>&>;
using ImageTextureDX                = TextureDX<ImageTextureArg>;

} // namespace Methane::Graphics
