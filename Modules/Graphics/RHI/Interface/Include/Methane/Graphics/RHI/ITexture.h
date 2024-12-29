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
#include <Methane/Graphics/Types.h>
#include <Methane/Memory.hpp>

namespace Methane::Graphics::Rhi
{

enum class TextureType : uint32_t
{
    Image = 0,
    RenderTarget,
    FrameBuffer,
    DepthStencil,
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

    using ResourceView::operator std::string;

    [[nodiscard]] const Ptr<ITexture>& GetTexturePtr() const noexcept { return m_texture_ptr; }
    [[nodiscard]] ITexture&            GetTexture() const;

private:
    // Resource::View stores pointer to the base class Resource, but pointer to ITexture is explicitly stored in ITexture::View too.
    // This is done to get rid of dynamic_cast type conversions, which would be required to get Ptr<ITexture> from Ptr<IResource> because of virtual inheritance
    Ptr<ITexture> m_texture_ptr;
};

using TextureViews = std::vector<TextureView>;

struct RenderContextSettings;

struct TextureSettings
{
    TextureType          type           = TextureType::Image;
    TextureDimensionType dimension_type = TextureDimensionType::Tex2D;
    ResourceUsageMask    usage_mask;
    PixelFormat          pixel_format   = PixelFormat::Unknown;
    Dimensions           dimensions     = {};
    uint32_t             array_length   = 1U;
    bool                 mipmapped      = false;

    // Optional settings for specific texture types
    Opt<Data::Index>        frame_index_opt;          // for TextureType::FrameBuffer
    Opt<DepthStencilValues> depth_stencil_clear_opt;  // for TextureType::DepthStencil

    friend bool operator==(const TextureSettings& left, const TextureSettings& right)
    {
        return std::tie(left.type, left.dimension_type, left.usage_mask, left.pixel_format, left.dimensions,
                        left.array_length, left.mipmapped, left.frame_index_opt, left.depth_stencil_clear_opt)
            == std::tie(right.type, right.dimension_type, right.usage_mask, right.pixel_format, right.dimensions,
                        right.array_length, right.mipmapped, right.frame_index_opt, right.depth_stencil_clear_opt);
    }

    friend bool operator!=(const TextureSettings& left, const TextureSettings& right)
    {
        return !(left == right);
    }

    [[nodiscard]] static TextureSettings ForImage(const Dimensions& dimensions, const Opt<uint32_t>& array_length_opt, PixelFormat pixel_format, bool mipmapped,
                                                  ResourceUsageMask usage = { ResourceUsage::ShaderRead });
    [[nodiscard]] static TextureSettings ForCubeImage(uint32_t dimension_size, const Opt<uint32_t>& array_length_opt, PixelFormat pixel_format, bool mipmapped,
                                                      ResourceUsageMask usage = { ResourceUsage::ShaderRead });
    [[nodiscard]] static TextureSettings ForFrameBuffer(const Dimensions& dimensions, PixelFormat pixel_format, Data::Index frame_index);
    [[nodiscard]] static TextureSettings ForFrameBuffer(const RenderContextSettings& render_context_settings, Data::Index frame_index);
    [[nodiscard]] static TextureSettings ForDepthStencil(const Dimensions& dimensions, PixelFormat pixel_format, const Opt<DepthStencilValues>& depth_stencil_clear,
                                                         ResourceUsageMask usage_mask = ResourceUsageMask(ResourceUsage::RenderTarget));
    [[nodiscard]] static TextureSettings ForDepthStencil(const RenderContextSettings& render_context_settings);
};

struct IRenderContext;

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
    [[nodiscard]] static Ptr<ITexture> Create(const IContext& context, const Settings& settings);

    // ITexture interface
    [[nodiscard]] virtual const Settings&    GetSettings() const = 0;
    [[nodiscard]] virtual Data::Size         GetSubResourceDataSize(const SubResource::Index& sub_resource_index = {}) const = 0;
    [[nodiscard]] virtual SubResource::Count GetSubresourceCount() const noexcept = 0;
    [[nodiscard]] virtual ResourceView       GetTextureView(const SubResource::Index& subresource_index,
                                                            const SubResource::Count& subresource_count = {},
                                                            Opt<TextureDimensionType> texture_dimension_type_opt = std::nullopt) = 0;
    [[nodiscard]] virtual SubResource        GetData(ICommandQueue& target_cmd_queue,
                                                     const SubResourceIndex& sub_resource_index = {},
                                                     const BytesRangeOpt& data_range = {}) = 0;
    virtual void SetData(ICommandQueue& target_cmd_queue, const SubResources& sub_resources) = 0;
};

} // namespace Methane::Graphics::Rhi
