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

FILE: Methane/Graphics/DirectX/Texture.h
DirectX 12 implementation of the texture interface.

******************************************************************************/

#pragma once

#include "Resource.hpp"

#include <Methane/Graphics/Base/Texture.h>
#include <Methane/Graphics/Base/CommandList.h>
#include <Methane/Graphics/Types.h>
#include <Methane/Graphics/Windows/DirectXErrorHandling.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <directx/d3dx12.h>
#include <optional>

namespace DirectX
{

class ScratchImage;

} // namespace DirectX

namespace Methane::Graphics::Base
{

class Context;

} // namespace Methane::Graphics::Base

namespace Methane::Graphics::DirectX
{

template<typename... ExtraArgs>
class Texture final // NOSONAR - inheritance hierarchy is greater than 5
    : public Resource<Base::Texture>
{
public:
    Texture(const Base::Context& context, const Settings& settings, ExtraArgs... extra_args)
        : Resource<Base::Texture>(context, settings)
    {
        META_FUNCTION_TASK();
        Initialize(extra_args...);
    }

    // IResource override
    void SetData(const SubResources&, Rhi::ICommandQueue&) override
    {
        META_FUNCTION_NOT_IMPLEMENTED_DESCR("Texture data upload is allowed for image textures only");
    }

    // IResource override
    Opt<Descriptor> InitializeNativeViewDescriptor(const View::Id& view_id) override;

private:
    void Initialize(ExtraArgs...);
};

using FrameBufferTexture = Texture<Rhi::ITexture::FrameBufferIndex>;

using RenderTargetTexture = Texture<>;
template<> class Texture<> final // NOSONAR - inheritance hierarchy is greater than 5
    : public Resource<Base::Texture>
{
public:
    Texture(const Base::Context& context, const Settings& settings);

    // IResource override
    Opt<Descriptor> InitializeNativeViewDescriptor(const View::Id& view_id) override;

private:
    void CreateShaderResourceView(const Descriptor& descriptor, const View::Id& view_id) const;
    void CreateRenderTargetView(const Descriptor& descriptor, const View::Id& view_id) const;
};

using DepthStencilTexture = Texture<const Opt<DepthStencil>&>;

template<> class Texture<const Opt<DepthStencil>&> final // NOSONAR - inheritance hierarchy is greater than 5
    : public Resource<Base::Texture>
{
public:
    Texture(const Base::Context& context, const Settings& settings, const Opt<DepthStencil>& clear_depth_stencil);

    // IResource override
    Opt<Descriptor> InitializeNativeViewDescriptor(const View::Id& view_id) override;

private:
    void CreateShaderResourceView(const Descriptor& descriptor) const;
    void CreateDepthStencilView(const Descriptor& descriptor) const;
};

struct ImageToken { };
using ImageTexture = Texture<ImageToken>;
template<> class Texture<ImageToken> final // NOSONAR - inheritance hierarchy is greater than 5
    : public Resource<Base::Texture>
{
public:
    Texture(const Base::Context& context, const Settings& settings, ImageToken);

    // IObject overrides
    bool SetName(const std::string& name) override;

    // IResource overrides
    void SetData(const SubResources& sub_resources, Rhi::ICommandQueue& target_cmd_queue) override;

    // IResource override
    Opt<Descriptor> InitializeNativeViewDescriptor(const View::Id& view_id) override;

private:
    void GenerateMipLevels(std::vector<D3D12_SUBRESOURCE_DATA>& dx_sub_resources, ::DirectX::ScratchImage& scratch_image) const;

    wrl::ComPtr<ID3D12Resource> m_cp_upload_resource;
};

} // namespace Methane::Graphics::DirectX
