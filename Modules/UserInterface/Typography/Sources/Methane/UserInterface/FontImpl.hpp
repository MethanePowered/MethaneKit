/******************************************************************************

Copyright 2020-2023 Evgeny Gorodetskiy

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

FILE: Methane/UserInterface/FontImpl.hpp
Font implementation.

******************************************************************************/

#pragma once

#include "FontChar.h"

#include <Methane/UserInterface/Font.h>
#include <Methane/UserInterface/FontLibrary.h>
#include <Methane/Graphics/RHI/RenderContext.h>
#include <Methane/Graphics/RHI/CommandKit.h>
#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/Texture.h>
#include <Methane/Graphics/Rect.hpp>
#include <Methane/Data/IProvider.h>
#include <Methane/Data/Emitter.hpp>

#include <map>
#include <string>
#include <ranges>
#include <cctype>
#include <cassert>

#include <ft2build.h>
#include <freetype/ftglyph.h>
#include FT_FREETYPE_H

namespace Methane::UserInterface
{

namespace gfx = Methane::Graphics;
namespace rhi = Methane::Graphics::Rhi;

inline void ThrowFreeTypeError(FT_Error error)
{
    if (error)
        throw FreeTypeError(error);
}

class Font::Impl // NOSONAR - class destructor is required, class has more than 35 methods
  : public Data::Emitter<IFontCallback>
  , protected Data::Receiver<rhi::IContextCallback> //NOSONAR
{
    struct AtlasTexture
    {
        rhi::Texture texture;
        bool is_update_required = true;
    };

    using Description = FontDescription;
    using Settings    = FontSettings;
    using Library     = FontLibrary;
    using Char        = FontChar;
    using CharBinPack = FontChar::BinPack;
    using Chars       = Refs<const Char>;
    using TextureByContext = std::map<rhi::RenderContext, AtlasTexture>;
    using CharByCode = std::map<Char::Code, Char>;

    class Face // NOSONAR - custom destructor is required
    {
    public:
        explicit Face(const Library& font_lib, Data::Chunk&& font_data)
            : m_font_data(std::move(font_data))
            , m_ft_face(LoadFace(font_lib.GetFreeTypeLibrary(), m_font_data))
            , m_ft_face_rec(GetFaceRec())
            , m_has_kerning(FT_HAS_KERNING(m_ft_face))
        { }

        ~Face()
        {
            META_FUNCTION_TASK();
            FT_Done_Face(m_ft_face);
        }

        Face(const Face&) noexcept = delete;
        Face(Face&&) noexcept = delete;

        Face& operator=(const Face&) noexcept = delete;
        Face& operator=(Face&&) noexcept = delete;

        void SetSize(uint32_t font_size_pt, uint32_t resolution_dpi)
        {
            META_FUNCTION_TASK();
            // 0 values mean that vertical value is equal to horizontal value
            ThrowFreeTypeError(FT_Set_Char_Size(m_ft_face, font_size_pt * s_ft_dots_in_pixel, 0, resolution_dpi, 0));
        }

        uint32_t GetCharIndex(Char::Code char_code)
        {
            META_FUNCTION_TASK();
            return FT_Get_Char_Index(m_ft_face, static_cast<FT_ULong>(char_code));
        }

        Char LoadChar(Char::Code char_code)
        {
            META_FUNCTION_TASK();

            uint32_t char_index = GetCharIndex(char_code);
            META_CHECK_NOT_ZERO_DESCR(char_index, "unicode character U+{} does not exist in font face", static_cast<uint32_t>(char_code));

            ThrowFreeTypeError(FT_Load_Glyph(m_ft_face, char_index, FT_LOAD_RENDER));
            META_CHECK_NOT_NULL_DESCR(m_ft_face_rec.glyph, "glyph should not be null after loading from font face");

            FT_Glyph ft_glyph = nullptr;
            ThrowFreeTypeError(FT_Get_Glyph(m_ft_face_rec.glyph, &ft_glyph));

            // All glyph metrics are multiplied by 64, so we reverse them back
            return Char(char_code,
                {
                    gfx::Point2I(),
                    gfx::FrameSize(static_cast<uint32_t>(m_ft_face_rec.glyph->metrics.width  / s_ft_dots_in_pixel),
                                   static_cast<uint32_t>(m_ft_face_rec.glyph->metrics.height / s_ft_dots_in_pixel))
                },
                gfx::Point2I(static_cast<int32_t>(m_ft_face_rec.glyph->metrics.horiBearingX  / s_ft_dots_in_pixel),
                             -static_cast<int32_t>(m_ft_face_rec.glyph->metrics.horiBearingY  / s_ft_dots_in_pixel)),
                gfx::Point2I(static_cast<int32_t>(m_ft_face_rec.glyph->metrics.horiAdvance   / s_ft_dots_in_pixel),
                             static_cast<int32_t>(m_ft_face_rec.glyph->metrics.vertAdvance   / s_ft_dots_in_pixel)),
                ft_glyph, char_index
            );
        }

        gfx::FramePoint GetKerning(uint32_t left_glyph_index, uint32_t right_glyph_index) const
        {
            META_FUNCTION_TASK();
            if (!m_has_kerning)
                return gfx::FramePoint(0, 0);

            META_CHECK_NOT_ZERO(left_glyph_index);
            META_CHECK_NOT_ZERO(right_glyph_index);

            FT_Vector kerning_vec{};
            ThrowFreeTypeError(FT_Get_Kerning(m_ft_face, left_glyph_index, right_glyph_index, FT_KERNING_DEFAULT, &kerning_vec));
            return gfx::FramePoint(static_cast<int>(kerning_vec.x >> 6), 0);
        }

        uint32_t GetLineHeight() const
        {
            META_FUNCTION_TASK();
            META_CHECK_NOT_NULL(m_ft_face_rec.size);
            return static_cast<uint32_t>(m_ft_face_rec.size->metrics.height / s_ft_dots_in_pixel);
        }

        const FT_FaceRec& GetFaceRec() const
        {
            META_FUNCTION_TASK();
            META_CHECK_NOT_NULL(m_ft_face);
            return *m_ft_face;
        }

    private:
        static FT_Face LoadFace(FT_Library ft_library, const Data::Chunk& font_data)
        {
            META_FUNCTION_TASK();
            FT_Face ft_face = nullptr;

            ThrowFreeTypeError(FT_New_Memory_Face(ft_library,
                                                  reinterpret_cast<const FT_Byte*>(font_data.GetDataPtr()), // NOSONAR
                                                  static_cast<FT_Long>(font_data.GetDataSize()), 0,
                                                  &ft_face));

            return ft_face;
        }

        const Data::Chunk m_font_data;
        const FT_Face     m_ft_face = nullptr;
        const FT_FaceRec& m_ft_face_rec;
        const bool        m_has_kerning;
    };

    Library                m_font_lib;
    Font&                  m_font;
    Settings               m_settings;
    Face                   m_face;
    UniquePtr<CharBinPack> m_atlas_pack_ptr;
    CharByCode             m_char_by_code;
    Data::Bytes            m_atlas_bitmap;
    TextureByContext       m_atlas_textures;
    gfx::FrameSize         m_max_glyph_size;

    static constexpr int32_t s_ft_dots_in_pixel = 64; // Freetype measures all font sizes in 1/64ths of pixels

public:

    Impl(const Library& font_lib, Font& font, const Data::IProvider& data_provider, const Settings& settings)
        : m_font_lib(font_lib)
        , m_font(font)
        , m_settings(settings)
        , m_face(font_lib, data_provider.GetData(m_settings.description.path))
    {
        META_FUNCTION_TASK();
        m_face.SetSize(m_settings.description.size_pt, m_settings.resolution_dpi);
        AddChars(m_settings.characters);
    }

    ~Impl() override
    {
        META_FUNCTION_TASK();
        try
        {
            ClearAtlasTextures();
        }
        catch(const std::exception& e)
        {
            META_UNUSED(e);
            META_LOG("WARNING: Unexpected error during Font destruction: {}", e.what());
            assert(false);
        }
    }

    const Library& GetLibrary() const
    {
        return m_font_lib;
    }

    [[nodiscard]] const Settings& GetSettings() const
    {
        return m_settings;
    }

    [[nodiscard]] const gfx::FrameSize& GetMaxGlyphSize() const noexcept
    {
        return m_max_glyph_size;
    }

    void ResetChars(const std::string& utf8_characters)
    {
        META_FUNCTION_TASK();
        ResetChars(ConvertUtf8To32(utf8_characters));
    }

    void ResetChars(const std::u32string& utf32_characters)
    {
        META_FUNCTION_TASK();
        m_atlas_pack_ptr.reset();
        m_char_by_code.clear();
        m_atlas_bitmap.clear();

        if (utf32_characters.empty())
        {
            for(const auto& [context_ptr, atlas_texture] : m_atlas_textures)
            {
                Emit(&IFontCallback::OnFontAtlasTextureReset, m_font, &atlas_texture.texture, nullptr);
            }
            m_atlas_textures.clear();
            return;
        }

        AddChars(utf32_characters);
        PackCharsToAtlas(1.2F);
        UpdateAtlasBitmap(false);
    }

    void AddChars(const std::string& utf8_characters)
    {
        META_FUNCTION_TASK();
        ResetChars(utf8_characters);
    }

    void AddChars(const std::u32string& utf32_characters)
    {
        META_FUNCTION_TASK();
        for (Char::Code char_code : utf32_characters)
        {
            if (!char_code)
                break;

            if (HasChar(char_code))
                continue;

            AddChar(char_code);
        }
    }

    const FontChar& AddChar(Char::Code char_code)
    {
        META_FUNCTION_TASK();
        if (const Char& font_char = GetChar(char_code); font_char)
            return font_char;

        // Load char glyph and add it to the font characters map
        const auto font_char_it = m_char_by_code.try_emplace(char_code, m_face.LoadChar(char_code)).first;
        META_CHECK_DESCR(static_cast<uint32_t>(char_code), font_char_it != m_char_by_code.end(), "font character was not added to character map");

        Char& new_font_char = font_char_it->second;
        m_max_glyph_size.SetWidth( std::max(m_max_glyph_size.GetWidth(),  new_font_char.GetRect().size.GetWidth()));
        m_max_glyph_size.SetHeight(std::max(m_max_glyph_size.GetHeight(), new_font_char.GetRect().size.GetHeight()));

        // Attempt to pack new char into existing atlas
        if (m_atlas_pack_ptr && m_atlas_pack_ptr->TryPack(new_font_char))
        {
            // Draw char to existing atlas bitmap and update textures
            new_font_char.DrawToAtlas(m_atlas_bitmap, m_atlas_pack_ptr->GetSize().GetWidth());
            UpdateAtlasTextures(true);
            return new_font_char;
        }

        // If new char does not fit into existing atlas, repack all chars into new atlas
        PackCharsToAtlas(2.F);
        UpdateAtlasBitmap(true);

        return new_font_char;
    }

    [[nodiscard]] bool HasChar(Char::Code char_code) const
    {
        META_FUNCTION_TASK();
        return m_char_by_code.contains(char_code) ||
               char_code == static_cast<Char::Code>('\n');
    }

    [[nodiscard]] const FontChar& GetChar(Char::Code char_code) const
    {
        META_FUNCTION_TASK();
        static const Char s_none_char {};
        if (static const Char s_line_break(static_cast<Char::Code>('\n'));
            char_code == s_line_break.GetCode())
            return s_line_break;

        const auto char_by_code_it = m_char_by_code.find(char_code);
        return char_by_code_it == m_char_by_code.end() ? s_none_char : char_by_code_it->second;
    }

    [[nodiscard]] Chars GetChars() const
    {
        META_FUNCTION_TASK();
        Chars font_chars;
        for(const auto& [char_code, character] : m_char_by_code)
        {
            font_chars.emplace_back(character);
        }
        return font_chars;
    }

    [[nodiscard]] Chars GetTextChars(const std::string& text)
    {
        META_FUNCTION_TASK();
        return GetTextChars(ConvertUtf8To32(text));
    }

    [[nodiscard]] Chars GetTextChars(const std::u32string& text)
    {
        META_FUNCTION_TASK();
        Refs<const Char> text_chars;
        text_chars.reserve(text.length());
        for (Char::Code char_code : text)
        {
            if (!char_code)
                break;

            text_chars.emplace_back(AddChar(char_code));
        }
        return text_chars;
    }

    gfx::FramePoint GetKerning(const Char& left_char, const Char& right_char) const
    {
        META_FUNCTION_TASK();
        return m_face.GetKerning(left_char.GetGlyphIndex(), right_char.GetGlyphIndex());
    }

    uint32_t GetLineHeight() const
    {
        META_FUNCTION_TASK();
        return m_face.GetLineHeight();
    }

    const gfx::FrameSize& GetAtlasSize() const noexcept
    {
        META_FUNCTION_TASK();
        static const gfx::FrameSize s_empty_size;
        return m_atlas_pack_ptr ? m_atlas_pack_ptr->GetSize() : s_empty_size;
    }

    const rhi::Texture& GetAtlasTexture(const rhi::RenderContext& context)
    {
        META_FUNCTION_TASK();
        META_CHECK_TRUE(context.IsInitialized());

        if (const auto atlas_texture_it = m_atlas_textures.find(context);
            atlas_texture_it != m_atlas_textures.end())
        {
            META_CHECK_TRUE(atlas_texture_it->second.texture.IsInitialized());
            return atlas_texture_it->second.texture;
        }

        static const rhi::Texture uninitialized_texture;
        if (m_char_by_code.empty())
            return uninitialized_texture;

        // Reserve 20% of pixels for packing space loss and for adding new characters to atlas
        if (!m_atlas_pack_ptr && !PackCharsToAtlas(1.2F))
            return uninitialized_texture;

        // Add font as context callback to remove atlas texture when context is released
        static_cast<Data::IEmitter<IContextCallback>&>(context.GetInterface()).Connect(*this);

        // Create atlas texture and render glyphs to it
        UpdateAtlasBitmap(true);

        const rhi::Texture& atlas_texture = m_atlas_textures.try_emplace(context, CreateAtlasTexture(context, true)).first->second.texture;
        Emit(&IFontCallback::OnFontAtlasTextureReset, m_font, nullptr, &atlas_texture);

        return atlas_texture;
    }

    void RemoveAtlasTexture(const rhi::RenderContext& render_context)
    {
        META_FUNCTION_TASK();
        m_atlas_textures.erase(render_context);
        static_cast<Data::IEmitter<IContextCallback>&>(render_context.GetInterface()).Disconnect(*this);
    }

    void ClearAtlasTextures()
    {
        META_FUNCTION_TASK();
        for(const auto& [context, atlas_texture] : m_atlas_textures)
        {
            if (!context.IsInitialized())
                continue;

            static_cast<Data::IEmitter<IContextCallback>&>(context.GetInterface()).Disconnect(*this);
            Emit(&IFontCallback::OnFontAtlasTextureReset, m_font, &atlas_texture.texture, nullptr);
        }
        m_atlas_textures.clear();
    }

private:
    Refs<FontChar> GetMutableChars()
    {
        META_FUNCTION_TASK();
        Refs<Char> font_chars;
        for(auto& [char_code, character] : m_char_by_code)
        {
            font_chars.emplace_back(character);
        }
        return font_chars;
    }

    bool PackCharsToAtlas(float pixels_reserve_multiplier)
    {
        META_FUNCTION_TASK();

        // Transform char-map to vector of chars
        Refs<Char> font_chars = GetMutableChars();
        if (font_chars.empty())
            return false;

        // Sort chars by decreasing of glyph pixels count from largest to smallest
        std::ranges::sort(font_chars,
            [](const Ref<Char>& left, const Ref<Char>& right)
            { return left.get() > right.get(); }
        );

        // Estimate required atlas size
        uint32_t char_pixels_count = 0U;
        for(const FontChar& font_char : font_chars)
        {
            char_pixels_count += font_char.GetRect().size.GetPixelsCount();
        }
        char_pixels_count = static_cast<uint32_t>(static_cast<float>(char_pixels_count) * pixels_reserve_multiplier);
        const auto square_atlas_dimension = static_cast<uint32_t>(std::sqrt(char_pixels_count));

        // Pack all character glyphs intro atlas size with doubling the size until all chars fit in
        gfx::FrameSize atlas_size(square_atlas_dimension, square_atlas_dimension);
        m_atlas_pack_ptr = std::make_unique<CharBinPack>(atlas_size);
        while(!m_atlas_pack_ptr->TryPack(font_chars))
        {
            atlas_size *= 2;
            m_atlas_pack_ptr = std::make_unique<CharBinPack>(atlas_size);
        }
        return true;
    }

    AtlasTexture CreateAtlasTexture(const rhi::RenderContext& render_context, bool deferred_data_init)
    {
        META_FUNCTION_TASK();
        rhi::Texture atlas_texture(render_context,
                                   rhi::TextureSettings::ForImage(
                                       gfx::Dimensions(m_atlas_pack_ptr->GetSize()),
                                       std::nullopt, gfx::PixelFormat::R8Unorm, false));
        atlas_texture.SetName(fmt::format("{} Font Atlas", m_settings.description.name));
        if (deferred_data_init)
        {
            render_context.RequestDeferredAction(rhi::IContext::DeferredAction::UploadResources);
        }
        else
        {
            atlas_texture.SetData(render_context.GetRenderCommandKit().GetQueue(),
                { rhi::IResource::SubResource(reinterpret_cast<Data::ConstRawPtr>(m_atlas_bitmap.data()), static_cast<Data::Size>(m_atlas_bitmap.size())) }); // NOSONAR
        }
        return { atlas_texture, deferred_data_init };
    }

    bool UpdateAtlasBitmap(bool deferred_textures_update)
    {
        META_FUNCTION_TASK();
        META_CHECK_NOT_NULL_DESCR(m_atlas_pack_ptr, "can not update atlas bitmap until atlas is packed");

        const gfx::FrameSize& atlas_size = m_atlas_pack_ptr->GetSize();
        if (m_atlas_bitmap.size() == atlas_size.GetPixelsCount())
            return false;

        // Clear old atlas content
        std::ranges::fill(m_atlas_bitmap, Data::Byte{});
        m_atlas_bitmap.resize(atlas_size.GetPixelsCount(), Data::Byte{});

        // Render glyphs to atlas bitmap
        for (const auto& [char_code, character] : m_char_by_code)
        {
            character.DrawToAtlas(m_atlas_bitmap, atlas_size.GetWidth());
        }

        UpdateAtlasTextures(deferred_textures_update);
        return true;
    }

    void UpdateAtlasTextures(bool deferred_textures_update)
    {
        META_FUNCTION_TASK();
        META_CHECK_NOT_NULL_DESCR(m_atlas_pack_ptr, "can not update atlas textures until atlas is packed and bitmap is up to date");
        if (m_atlas_textures.empty())
            return;

        for(auto& [context, atlas_texture] : m_atlas_textures)
        {
            if (deferred_textures_update)
            {
                // Texture will be updated on GPU context completing initialization,
                // when next GPU Frame rendering is started and just before uploading data on GPU with upload command queue
                atlas_texture.is_update_required = true;
                context.RequestDeferredAction(rhi::IContext::DeferredAction::UploadResources);
            }
            else
            {
                META_CHECK_TRUE(context.IsInitialized());
                UpdateAtlasTexture(context, atlas_texture);
            }
        }

        Emit(&IFontCallback::OnFontAtlasUpdated, m_font);
    }

    void UpdateAtlasTexture(const rhi::RenderContext& render_context, AtlasTexture& atlas_texture)
    {
        META_FUNCTION_TASK();
        META_CHECK_TRUE_DESCR(atlas_texture.texture.IsInitialized(), "font atlas texture is not initialized");

        const gfx::FrameSize atlas_size = m_atlas_pack_ptr->GetSize();
        if (const gfx::Dimensions& texture_dimensions = atlas_texture.texture.GetSettings().dimensions;
            texture_dimensions.GetWidth() != atlas_size.GetWidth() || texture_dimensions.GetHeight() != atlas_size.GetHeight())
        {
            const rhi::Texture old_texture = atlas_texture.texture;
            atlas_texture.texture = CreateAtlasTexture(render_context, false).texture;
            Emit(&IFontCallback::OnFontAtlasTextureReset, m_font, &old_texture, &atlas_texture.texture);
        }
        else
        {
            atlas_texture.texture.SetData(render_context.GetRenderCommandKit().GetQueue(),
                { rhi::IResource::SubResource(reinterpret_cast<Data::ConstRawPtr>(m_atlas_bitmap.data()), static_cast<Data::Size>(m_atlas_bitmap.size())) }); // NOSONAR
        }

        atlas_texture.is_update_required = false;
    }

    void OnContextReleased(rhi::IContext& context) final
    {
        META_FUNCTION_TASK();
        META_CHECK_EQUAL(context.GetType(), rhi::IContext::Type::Render);
        RemoveAtlasTexture(rhi::RenderContext(dynamic_cast<rhi::IRenderContext&>(context)));
    }

    void OnContextUploadingResources(rhi::IContext& context) final
    {
        META_FUNCTION_TASK();
        META_CHECK_EQUAL(context.GetType(), rhi::IContext::Type::Render);
        const rhi::RenderContext render_context(dynamic_cast<rhi::IRenderContext&>(context));
        if (const auto atlas_texture_it = m_atlas_textures.find(render_context);
            atlas_texture_it != m_atlas_textures.end() && atlas_texture_it->second.is_update_required)
        {
            UpdateAtlasTexture(render_context, atlas_texture_it->second);
        }
    }

    void OnContextInitialized(rhi::IContext&) final
    {
        // Intentionally unimplemented
    }
};

} // namespace Methane::UserInterface
