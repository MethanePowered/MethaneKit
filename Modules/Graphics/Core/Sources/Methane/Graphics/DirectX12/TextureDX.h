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
    TextureDX(const ContextBase& context, const Settings& settings, ExtraArgs... extra_args)
        : ResourceDX<TextureBase>(context, settings)
    {
        META_FUNCTION_TASK();
        Initialize(extra_args...);
    }

    // Resource override
    void SetData(const SubResources&, CommandQueue&) override
    {
        META_FUNCTION_NOT_IMPLEMENTED_DESCR("Texture data setup is allowed for image textures only");
    }

    // IResourceDX override
    Opt<Descriptor> InitializeNativeViewDescriptor(const LocationDX::Id& location_id) override;

private:
    void Initialize(ExtraArgs...);
};

struct ImageTokenDX { };

template<>
class TextureDX<ImageTokenDX> final : public ResourceDX<TextureBase>
{
public:
    TextureDX(const ContextBase& context, const Settings& settings, ImageTokenDX);

    // Object overrides
    bool SetName(const std::string& name) override;

    // Resource overrides
    void SetData(const SubResources& sub_resources, CommandQueue& target_cmd_queue) override;

    // IResourceDX override
    Opt<Descriptor> InitializeNativeViewDescriptor(const LocationDX::Id& location_id) override;

private:
    void GenerateMipLevels(std::vector<D3D12_SUBRESOURCE_DATA>& dx_sub_resources, DirectX::ScratchImage& scratch_image) const;

    wrl::ComPtr<ID3D12Resource> m_cp_upload_resource;
};

template<>
class TextureDX<const Opt<DepthStencil>&> final : public ResourceDX<TextureBase>
{
public:
    TextureDX(const ContextBase& context, const Settings& settings, const Opt<DepthStencil>& clear_depth_stencil);

    // Resource overrides
    void SetData(const SubResources&, CommandQueue&) override
    {
        META_FUNCTION_NOT_IMPLEMENTED_DESCR("depth stencil texture does not allow to set data");
    }

    // IResourceDX override
    Opt<Descriptor> InitializeNativeViewDescriptor(const LocationDX::Id& location_id) override;

private:
    void CreateShaderResourceView(const D3D12_CPU_DESCRIPTOR_HANDLE& cpu_descriptor_handle) const;
    void CreateDepthStencilView(const D3D12_CPU_DESCRIPTOR_HANDLE& cpu_descriptor_handle) const;
};

using FrameBufferTextureDX  = TextureDX<Texture::FrameBufferIndex>;
using RenderTargetTextureDX = TextureDX<>;
using DepthStencilTextureDX = TextureDX<const Opt<DepthStencil>&>;
using ImageTextureDX        = TextureDX<ImageTokenDX>;

} // namespace Methane::Graphics
