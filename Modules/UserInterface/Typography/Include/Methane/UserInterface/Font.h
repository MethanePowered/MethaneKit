/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/UserInterface/Font.h
Font atlas textures generation and fonts library management classes.

******************************************************************************/

#pragma once

#include <Methane/Graphics/RenderContext.h>
#include <Methane/Graphics/Rect.hpp>
#include <Methane/Data/Provider.h>
#include <Methane/Data/Emitter.hpp>

#include <magic_enum.hpp>
#include <map>
#include <string>
#include <cctype>
#include <stdexcept>

namespace Methane::Graphics
{
struct Texture;
}

namespace Methane::UserInterface
{

namespace gfx = Methane::Graphics;

class Font;

struct IFontLibraryCallback
{
    virtual void OnFontAdded(Font& font) = 0;
    virtual void OnFontRemoved(Font& font) = 0;

    virtual ~IFontLibraryCallback() = default;
};

struct IFontCallback
{
    virtual void OnFontAtlasTextureReset(Font& font, const Ptr<gfx::Texture>& old_atlas_texture_ptr, const Ptr<gfx::Texture>& new_atlas_texture_ptr) = 0;
    virtual void OnFontAtlasUpdated(Font& font) = 0;

    virtual ~IFontCallback() = default;
};

#ifndef FT_Error
using FT_Error = int;
#endif

class Font
    : public std::enable_shared_from_this<Font>
    , public Data::Emitter<IFontCallback>
    , protected Data::Receiver<gfx::IContextCallback> //NOSONAR
{
public:
    struct Description
    {
        std::string name;
        std::string path;
        uint32_t    size_pt;
    };

    struct Settings
    {
        Description    description;
        uint32_t       resolution_dpi;
        std::u32string characters;
    };

    class FreeTypeError : public std::runtime_error
    {
    public:
        explicit FreeTypeError(FT_Error error);

        [[nodiscard]] FT_Error GetError() const noexcept { return m_error; }

    private:
        const FT_Error m_error;
    };

    class Library
        : public Data::Emitter<IFontLibraryCallback>
    {
        friend class Font;

    public:
        [[nodiscard]] static Library& Get();

        [[nodiscard]] Refs<Font> GetFonts() const;
        [[nodiscard]] bool  HasFont(const std::string& font_name) const;
        [[nodiscard]] Font& GetFont(const std::string& font_name) const;
        [[nodiscard]] Font& GetFont(const Data::Provider& data_provider, const Settings& font_settings);
        Font& AddFont(const Data::Provider& data_provider, const Settings& font_settings);
        void  RemoveFont(const std::string& font_name);
        void  Clear();

    protected:
        class Impl;
        [[nodiscard]] Impl& GetImpl() { return *m_impl_ptr; }

    private:
        Library();

        using FontByName = std::map<std::string, Ptr<Font>, std::less<>>;

        const UniquePtr<Impl> m_impl_ptr;
        FontByName            m_font_by_name;
    };

    class CharBinPack;

    class Char
    {
        friend class CharBinPack;

    public:
        class Glyph;
        using Code = char32_t;

        enum class Type : uint8_t
        {
            Unknown    = 0U,
            Whitespace = 1U << 0U,
            LineBreak  = 1U << 1U
        };

        static Type GetType(Code code);

        Char() = default;
        explicit Char(Code code);
        Char(Code code, gfx::FrameRect rect, gfx::Point2I offset, gfx::Point2I advance, UniquePtr<Glyph>&& glyph_ptr);

        [[nodiscard]] Code                  GetCode() const noexcept        { return m_code; }
        [[nodiscard]] bool                  IsLineBreak() const noexcept    { using namespace magic_enum::bitwise_operators;
                                                                              return magic_enum::flags::enum_contains(m_type_mask & Type::LineBreak); }
        [[nodiscard]] bool                  IsWhiteSpace() const noexcept   { using namespace magic_enum::bitwise_operators;
                                                                              return magic_enum::flags::enum_contains(m_type_mask & Type::Whitespace); }
        [[nodiscard]] const gfx::FrameRect& GetRect() const noexcept        { return m_rect; }
        [[nodiscard]] const gfx::Point2I&   GetOffset() const noexcept      { return m_offset; }
        [[nodiscard]] const gfx::Point2I&   GetAdvance() const noexcept     { return m_advance; }
        [[nodiscard]] const gfx::FrameSize& GetVisualSize() const noexcept  { return m_visual_size; }

        [[nodiscard]] bool operator<(const Char& other) const noexcept      { return m_rect.size.GetPixelsCount() < other.m_rect.size.GetPixelsCount(); }
        [[nodiscard]] bool operator>(const Char& other) const noexcept      { return m_rect.size.GetPixelsCount() > other.m_rect.size.GetPixelsCount(); }
        [[nodiscard]] explicit operator bool() const noexcept               { return m_code != 0U; }

        void     DrawToAtlas(Data::Bytes& atlas_bitmap, uint32_t atlas_row_stride) const;
        uint32_t GetGlyphIndex() const;

    private:
        const Code       m_code = 0U;
        const Type       m_type_mask = Type::Unknown;
        gfx::FrameRect   m_rect;
        gfx::Point2I     m_offset;
        gfx::Point2I     m_advance;
        gfx::FrameSize   m_visual_size;
        UniquePtr<Glyph> m_glyph_ptr;
    };

    using Chars = Refs<const Char>;

    [[nodiscard]] static std::u32string ConvertUtf8To32(std::string_view text);
    [[nodiscard]] static std::string    ConvertUtf32To8(std::u32string_view text);
    [[nodiscard]] static std::u32string GetAlphabetDefault() { return GetAlphabetInRange(32, 126); }
    [[nodiscard]] static std::u32string GetAlphabetInRange(char32_t from, char32_t to);
    [[nodiscard]] static std::u32string GetAlphabetFromText(const std::string& text);
    [[nodiscard]] static std::u32string GetAlphabetFromText(const std::u32string& text);

    ~Font() override;

    [[nodiscard]] Ptr<Font>       GetPtr()            { return shared_from_this(); }
    [[nodiscard]] const Settings& GetSettings() const { return m_settings; }

    void                     ResetChars(const std::string& utf8_characters);
    void                     ResetChars(const std::u32string& utf32_characters);
    void                     AddChars(const std::string& utf8_characters);
    void                     AddChars(const std::u32string& utf32_characters);
    const Font::Char&        AddChar(Char::Code char_code);
    
    [[nodiscard]] bool                     HasChar(Char::Code char_code) const;
    [[nodiscard]] const Char&              GetChar(Char::Code char_code) const;
    [[nodiscard]] Chars                    GetChars() const;
    [[nodiscard]] Chars                    GetTextChars(const std::string& text);
    [[nodiscard]] Chars                    GetTextChars(const std::u32string& text);
    [[nodiscard]] gfx:: FramePoint         GetKerning(const Char& left_char, const Char& right_char) const;
    [[nodiscard]] uint32_t                 GetLineHeight() const;
    [[nodiscard]] const gfx::FrameSize&    GetMaxGlyphSize() const noexcept { return m_max_glyph_size; }
    [[nodiscard]] const gfx::FrameSize&    GetAtlasSize() const noexcept;
    [[nodiscard]] const Ptr<gfx::Texture>& GetAtlasTexturePtr(gfx::RenderContext& context);
    [[nodiscard]] gfx::Texture&            GetAtlasTexture(gfx::RenderContext& context);

protected:
    // Font can be created only via Font::Library::Add
    Font(const Data::Provider& data_provider, const Settings& settings);

    Refs<Char> GetMutableChars();
    bool PackCharsToAtlas(float pixels_reserve_multiplier);

    // IContextCallback interface
    void OnContextReleased(gfx::Context& context) override;
    void OnContextCompletingInitialization(gfx::Context& context) override;
    void OnContextInitialized(gfx::Context&) override { /* callback not handled in this class */}

private:
    struct AtlasTexture
    {
        Ptr<gfx::Texture> texture_ptr;
        bool              is_update_required = true;
    };

    AtlasTexture CreateAtlasTexture(const gfx::RenderContext& context, bool deferred_data_init);
    void RemoveAtlasTexture(gfx::RenderContext& context);

    bool UpdateAtlasBitmap(bool deferred_textures_update);
    void UpdateAtlasTextures(bool deferred_textures_update);
    void UpdateAtlasTexture(const gfx::RenderContext& context, AtlasTexture& atlas_texture);
    void ClearAtlasTextures();

    class Face;
    using TextureByContext = std::map<gfx::RenderContext*, AtlasTexture>;
    using CharByCode = std::map<Char::Code, Char>;

    Settings               m_settings;
    UniquePtr<Face>        m_face_ptr;
    UniquePtr<CharBinPack> m_atlas_pack_ptr;
    CharByCode             m_char_by_code;
    Data::Bytes            m_atlas_bitmap;
    TextureByContext       m_atlas_textures;
    gfx::FrameSize         m_max_glyph_size;
};

} // namespace Methane::Graphics
