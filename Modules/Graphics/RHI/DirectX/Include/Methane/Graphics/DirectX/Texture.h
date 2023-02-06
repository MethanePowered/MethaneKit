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

#include <directx/d3d12.h>

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

class Texture final // NOSONAR - inheritance hierarchy is greater than 5
    : public Resource<Base::Texture>
{
public:
    using View = ResourceView;

    Texture(const Base::Context& context, const Settings& settings);

    // IObject overrides
    bool SetName(std::string_view name) override;

    // IResource override
    void SetData(const SubResources&, Rhi::ICommandQueue&) override;

    // IResource override
    Opt<Descriptor> InitializeNativeViewDescriptor(const View::Id& view_id) override;

private:
    void InitializeAsImage();
    void InitializeAsRenderTarget();
    void InitializeAsFrameBuffer();
    void InitializeAsDepthStencil();

    void CreateShaderResourceView(const Descriptor& descriptor) const;
    void CreateShaderResourceView(const Descriptor& descriptor, const View::Id& view_id) const;
    void CreateRenderTargetView(const Descriptor& descriptor) const;
    void CreateRenderTargetView(const Descriptor& descriptor, const View::Id& view_id) const;
    void CreateDepthStencilView(const Descriptor& descriptor) const;
    void GenerateMipLevels(std::vector<D3D12_SUBRESOURCE_DATA>& dx_sub_resources, ::DirectX::ScratchImage& scratch_image) const;

    // Upload resource is created for TextureType::Image only
    wrl::ComPtr<ID3D12Resource> m_cp_upload_resource;
};

} // namespace Methane::Graphics::DirectX
