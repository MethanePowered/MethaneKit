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

struct Texture : virtual Resource
{
    enum class Type : uint32_t
    {
        Texture = 0,
        FrameBuffer,
        DepthStencilBuffer,
    };

    enum class DimensionType : uint32_t
    {
        Tex1D = 0,
        Tex1DArray,
        Tex2D,
        Tex2DArray,
        Tex2DMultisample,
        Cube,
        CubeArray,
        Tex3D,
    };

    class Location : public Resource::Location
    {
    public:
        Location() = default;
        Location(const Ptr<Texture>& texture_ptr, const SubResource::Index& subresource_index = SubResource::Index())
            : Resource::Location(texture_ptr, subresource_index)
            , m_texture_ptr(texture_ptr)
        { }

        using Resource::Location::operator==;

        const Ptr<Texture>& GetTexturePtr() const noexcept { return m_texture_ptr; }
        Texture&            GetTexture() const;

    private:
        // Resource::Location stores pointer to the base class Resource, but pointer to Texture is explicitly stored in Texture::Location too
        // This is done to get rid of dynamic_cast type conversions, which would be required to get Ptr<Texture> from Ptr<Resource> because of virtual inheritance
        Ptr<Texture> m_texture_ptr;
    };

    struct Settings
    {
        Type           type           = Type::Texture;
        DimensionType  dimension_type = DimensionType::Tex2D;
        Usage          usage_mask     = Usage::None;
        PixelFormat    pixel_format   = PixelFormat::Unknown;
        Dimensions     dimensions     = Dimensions();
        uint32_t       array_length   = 1U;
        bool           mipmapped      = false;

        static Settings Image(const Dimensions& dimensions, uint32_t array_length, PixelFormat pixel_format, bool mipmapped, Usage usage);
        static Settings Cube(uint32_t dimension_size, uint32_t array_length, PixelFormat pixel_format, bool mipmapped, Usage usage);
        static Settings FrameBuffer(const Dimensions& dimensions, PixelFormat pixel_format);
        static Settings DepthStencilBuffer(const Dimensions& dimensions, PixelFormat pixel_format, Usage usage_mask = Usage::RenderTarget);
    };

    // Create Texture instance
    static Ptr<Texture> CreateRenderTarget(RenderContext& context, const Settings& settings,
                                           const DescriptorByUsage& descriptor_by_usage = DescriptorByUsage());
    static Ptr<Texture> CreateFrameBuffer(RenderContext& context, uint32_t frame_buffer_index,
                                          const DescriptorByUsage& descriptor_by_usage = DescriptorByUsage());
    static Ptr<Texture> CreateDepthStencilBuffer(RenderContext& context,
                                                 const DescriptorByUsage& descriptor_by_usage = DescriptorByUsage());
    static Ptr<Texture> CreateImage(Context& context, const Dimensions& dimensions, uint32_t array_length, PixelFormat pixel_format, bool mipmapped,
                                    const DescriptorByUsage& descriptor_by_usage = DescriptorByUsage());
    static Ptr<Texture> CreateCube(Context& context, uint32_t dimension_size, uint32_t array_length, PixelFormat pixel_format, bool mipmapped,
                                   const DescriptorByUsage& descriptor_by_usage = DescriptorByUsage());

    // Texture interface
    virtual const Settings& GetSettings() const = 0;
};

} // namespace Methane::Graphics
