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

FILE: Methane/Graphics/Texture.h
Methane graphics interface: graphics texture.

******************************************************************************/

#pragma once

#include "Resource.h"

#include <Methane/Graphics/Volume.hpp>
#include <Methane/Graphics/RenderContext.h>
#include <Methane/Memory.hpp>

namespace Methane::Graphics
{

struct Texture : virtual Resource // NOSONAR
{
    using DimensionType = TextureDimensionType;

    enum class Type : uint32_t
    {
        Texture = 0,
        FrameBuffer,
        DepthStencilBuffer,
    };

    class View : public Resource::View
    {
    public:
        View(Texture& texture, const SubResource::Index& subresource_index = {}, const SubResource::Count& subresource_count = {},
             Opt<TextureDimensionType> texture_dimension_type_opt = {});

        using Resource::View::operator==;
        using Resource::View::operator!=;
        using Resource::View::operator std::string;

        [[nodiscard]] const Ptr<Texture>& GetTexturePtr() const noexcept { return m_texture_ptr; }
        [[nodiscard]] Texture&            GetTexture() const;

    private:
        // Resource::View stores pointer to the base class Resource, but pointer to Texture is explicitly stored in Texture::View too.
        // This is done to get rid of dynamic_cast type conversions, which would be required to get Ptr<Texture> from Ptr<Resource> because of virtual inheritance
        Ptr<Texture> m_texture_ptr;
    };

    using Views = std::vector<View>;

    struct Settings
    {
        Type           type           = Type::Texture;
        DimensionType  dimension_type = DimensionType::Tex2D;
        Usage          usage_mask     = Usage::None;
        PixelFormat    pixel_format   = PixelFormat::Unknown;
        Dimensions     dimensions     = Dimensions();
        uint32_t       array_length   = 1U;
        bool           mipmapped      = false;

        [[nodiscard]] static Settings Image(const Dimensions& dimensions, const Opt<uint32_t>& array_length_opt, PixelFormat pixel_format, bool mipmapped, Usage usage);
        [[nodiscard]] static Settings Cube(uint32_t dimension_size, const Opt<uint32_t>& array_length_opt, PixelFormat pixel_format, bool mipmapped, Usage usage);
        [[nodiscard]] static Settings FrameBuffer(const Dimensions& dimensions, PixelFormat pixel_format);
        [[nodiscard]] static Settings DepthStencilBuffer(const Dimensions& dimensions, PixelFormat pixel_format, Usage usage_mask = Usage::RenderTarget);
    };

    using FrameBufferIndex = uint32_t;

    // Create Texture instance
    [[nodiscard]] static Ptr<Texture> CreateRenderTarget(const RenderContext& context, const Settings& settings);
    [[nodiscard]] static Ptr<Texture> CreateFrameBuffer(const RenderContext& context, FrameBufferIndex frame_buffer_index);
    [[nodiscard]] static Ptr<Texture> CreateDepthStencilBuffer(const RenderContext& context);
    [[nodiscard]] static Ptr<Texture> CreateImage(const Context& context, const Dimensions& dimensions, const Opt<uint32_t>& array_length_opt, PixelFormat pixel_format, bool mipmapped);
    [[nodiscard]] static Ptr<Texture> CreateCube(const Context& context, uint32_t dimension_size, const Opt<uint32_t>& array_length_opt, PixelFormat pixel_format, bool mipmapped);

    // Texture interface
    [[nodiscard]] virtual const Settings& GetSettings() const = 0;
};

} // namespace Methane::Graphics
