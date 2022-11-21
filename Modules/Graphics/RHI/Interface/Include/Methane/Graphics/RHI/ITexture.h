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

FILE: Methane/Graphics/RHI/ITexture.h
Methane graphics interface: graphics texture.

******************************************************************************/

#pragma once

#include "IResource.h"

#include <Methane/Graphics/Volume.hpp>
#include <Methane/Graphics/RHI/IRenderContext.h>
#include <Methane/Memory.hpp>

namespace Methane::Graphics::Rhi
{

enum class TextureType : uint32_t
{
    Texture = 0,
    FrameBuffer,
    DepthStencilBuffer,
};

struct ITexture;

class TextureView
    : public ResourceView
{
public:
    TextureView(ITexture& texture,
                const SubResource::Index& subresource_index = {},
                const SubResource::Count& subresource_count = {},
                Opt<TextureDimensionType> texture_dimension_type_opt = {});

    using ResourceView::operator==;
    using ResourceView::operator!=;
    using ResourceView::operator std::string;

    [[nodiscard]] const Ptr<ITexture>& GetTexturePtr() const noexcept { return m_texture_ptr; }
    [[nodiscard]] ITexture&            GetTexture() const;

private:
    // Resource::View stores pointer to the base class Resource, but pointer to ITexture is explicitly stored in ITexture::View too.
    // This is done to get rid of dynamic_cast type conversions, which would be required to get Ptr<ITexture> from Ptr<IResource> because of virtual inheritance
    Ptr<ITexture> m_texture_ptr;
};

using TextureViews = std::vector<TextureView>;

struct TextureSettings
{
    TextureType           type           = TextureType::Texture;
    TextureDimensionType  dimension_type = TextureDimensionType::Tex2D;
    ResourceUsage         usage_mask;
    PixelFormat           pixel_format   = PixelFormat::Unknown;
    Dimensions            dimensions     = {};
    uint32_t              array_length   = 1U;
    bool                  mipmapped      = false;

    [[nodiscard]] static TextureSettings Image(const Dimensions& dimensions, const Opt<uint32_t>& array_length_opt, PixelFormat pixel_format, bool mipmapped, ResourceUsage usage);
    [[nodiscard]] static TextureSettings Cube(uint32_t dimension_size, const Opt<uint32_t>& array_length_opt, PixelFormat pixel_format, bool mipmapped, ResourceUsage usage);
    [[nodiscard]] static TextureSettings FrameBuffer(const Dimensions& dimensions, PixelFormat pixel_format);
    [[nodiscard]] static TextureSettings DepthStencilBuffer(const Dimensions& dimensions, PixelFormat pixel_format,
                                                            ResourceUsage usage_mask = ResourceUsage({ ResourceUsage::Bit::RenderTarget }));
};

struct ITexture
    : virtual IResource // NOSONAR
{
    using Type             = TextureType;
    using DimensionType    = TextureDimensionType;
    using View             = TextureView;
    using Views            = TextureViews;
    using Settings         = TextureSettings;
    using FrameBufferIndex = uint32_t;

    // Create ITexture instance
    [[nodiscard]] static Ptr<ITexture> CreateRenderTarget(const IRenderContext& context, const Settings& settings);
    [[nodiscard]] static Ptr<ITexture> CreateFrameBuffer(const IRenderContext& context, FrameBufferIndex frame_buffer_index);
    [[nodiscard]] static Ptr<ITexture> CreateDepthStencilBuffer(const IRenderContext& context);
    [[nodiscard]] static Ptr<ITexture> CreateImage(const IContext& context, const Dimensions& dimensions, const Opt<uint32_t>& array_length_opt, PixelFormat pixel_format, bool mipmapped);
    [[nodiscard]] static Ptr<ITexture> CreateCube(const IContext& context, uint32_t dimension_size, const Opt<uint32_t>& array_length_opt, PixelFormat pixel_format, bool mipmapped);

    // ITexture interface
    [[nodiscard]] virtual const Settings& GetSettings() const = 0;
};

} // namespace Methane::Graphics::Rhi
