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

FILE: Methane/Graphics/DirectX12/TextureDX.h
DirectX 12 implementation of the texture interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/TextureBase.h>
#include <Methane/Graphics/CommandListBase.h>
#include <Methane/Graphics/Types.h>
#include <Methane/Graphics/Windows/Helpers.h>
#include <Methane/Instrumentation.h>

#include <d3dx12.h>

#include <optional>
#include <cassert>

namespace DirectX
{
class ScratchImage;
}

namespace Methane::Graphics
{

template<typename... ExtraArgs>
class TextureDX : public TextureBase
{
public:
    TextureDX(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage, ExtraArgs... extra_args)
        : TextureBase(context, settings, descriptor_by_usage)
    {
        ITT_FUNCTION_TASK();
        InitializeDefaultDescriptors();
        Initialize(extra_args...);
    }

    ~TextureDX() override
    {
        ITT_FUNCTION_TASK();
    }

    // Resource interface
    void SetData(const SubResources&) override
    {
        ITT_FUNCTION_TASK();
        throw std::logic_error("Setting texture data is allowed for image textures only.");
    }

    Data::Size GetDataSize() const override
    {
        ITT_FUNCTION_TASK();
        const Settings& settings = GetSettings();
        return settings.dimensions.GetPixelsCount() * GetPixelSize(settings.pixel_format);
    }

private:
    void Initialize(ExtraArgs...);
};

struct ImageTextureArg { };

template<>
class TextureDX<ImageTextureArg> : public TextureBase
{
public:
    TextureDX(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage, ImageTextureArg);

    // Resource interface
    void SetData(const SubResources& sub_resources) override;
    Data::Size GetDataSize() const override { return m_data_size; }

protected:
    using ResourceAndViewDesc = std::pair<D3D12_RESOURCE_DESC, D3D12_SHADER_RESOURCE_VIEW_DESC>;
    ResourceAndViewDesc GetResourceAndViewDesc() const;
    void GenerateMipLevels(std::vector<D3D12_SUBRESOURCE_DATA>& dx_sub_resources, DirectX::ScratchImage& scratch_image);

    Data::Size                  m_data_size = 0;
    wrl::ComPtr<ID3D12Resource> m_cp_upload_resource;
};

using RenderTargetTextureDX         = TextureDX<>;
using FrameBufferTextureDX          = TextureDX<uint32_t /* frame_buffer_index */>;
using DepthStencilBufferTextureDX   = TextureDX<const std::optional<DepthStencil>&>;
using ImageTextureDX                = TextureDX<ImageTextureArg>;

} // namespace Methane::Graphics
