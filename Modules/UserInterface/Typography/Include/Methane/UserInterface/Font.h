/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/UserInterface/Font.h
Font atlas textures generation and fonts library management classes.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Context.h>
#include <Methane/Graphics/Rect.hpp>
#include <Methane/Data/Emitter.hpp>

#include <map>
#include <string>
#include <cctype>

namespace Methane::Data
{
struct Provider;
}

namespace Methane::Graphics
{
struct Texture;
}

namespace Methane::UserInterface
{

namespace gfx = Methane::Graphics;

class Font;

struct IFontCallback
{
    virtual void OnFontAtlasTextureReset(Font& font, const Ptr<gfx::Texture>& sp_old_atlas_texture, const Ptr<gfx::Texture>& sp_new_atlas_texture) = 0;
    virtual void OnFontAtlasUpdated(Font& font) = 0;

    virtual ~IFontCallback() = default;
};

class Font
    : public std::enable_shared_from_this<Font>
    , public Data::Emitter<IFontCallback>
    , protected Data::Receiver<gfx::IContextCallback>
{
public:
    struct Settings
    {
        std::string    name;
        std::string    font_path;
        uint32_t       font_size_pt;
        uint32_t       resolution_dpi;
        std::u32string characters;
    };

    class Library
    {
        friend class Font;

    public:
        static Library& Get();

        Refs<Font> GetFonts() const;
        bool  HasFont(const std::string& font_name) const;
        Font& GetFont(const std::string& font_name) const;
        Font& GetFont(const Data::Provider& data_provider, const Settings& font_settings);
        Font& AddFont(const Data::Provider& data_provider, const Settings& font_settings);
        void  RemoveFont(const std::string& font_name);
        void  Clear();

    protected:
        class Impl;
        Impl& GetImpl() { return *m_sp_impl; }

    private:
        Library();

        using FontByName = std::map<std::string, Ptr<Font>>;

        const UniquePtr<Impl> m_sp_impl;
        FontByName            m_font_by_name;
    };

    class CharBinPack;

    class Char
    {
        friend class CharBinPack;

    public:
        class Glyph;
        using Code = char32_t;

        struct Type
        {
            using Mask = uint8_t;
            enum Value : Mask
            {
                Unknown    = 0u,
                Whitespace = 1u << 0u,
                LineBreak  = 1u << 1u,
            };

            static Mask Get(Code code);

            Type() = delete;
        };

        Char() = default;
        Char(Code code);
        Char(Code code, gfx::FrameRect rect, gfx::Point2i offset, gfx::Point2i advance, UniquePtr<Glyph>&& sp_glyph);

        Code                  GetCode() const noexcept        { return m_code; }
        bool                  IsLineBreak() const noexcept    { return m_type_mask & Type::LineBreak; }
        bool                  IsWhiteSpace() const noexcept   { return m_type_mask & Type::Whitespace; }
        const gfx::FrameRect& GetRect() const noexcept        { return m_rect; }
        const gfx::Point2i&   GetOffset() const noexcept      { return m_offset; }
        const gfx::Point2i&   GetAdvance() const noexcept     { return m_advance; }

        bool operator<(const Char& other) const noexcept      { return m_rect.size.GetPixelsCount() < other.m_rect.size.GetPixelsCount(); }
        bool operator>(const Char& other) const noexcept      { return m_rect.size.GetPixelsCount() > other.m_rect.size.GetPixelsCount(); }
        operator bool() const noexcept                        { return m_code != 0u; }

        void     DrawToAtlas(Data::Bytes& atlas_bitmap, uint32_t atlas_row_stride) const;
        uint32_t GetGlyphIndex() const;

    private:
        const Code       m_code = 0u;
        const Type::Mask m_type_mask = Type::Value::Unknown;
        gfx::FrameRect   m_rect;
        gfx::Point2i     m_offset;
        gfx::Point2i     m_advance;
        UniquePtr<Glyph> m_sp_glyph;
    };

    using Chars = Refs<const Char>;

    static std::u32string ConvertUtf8To32(const std::string& text);
    static std::string    ConvertUtf32To8(const std::u32string& text);
    static std::u32string GetAlphabetDefault() { return GetAlphabetInRange(32, 126); }
    static std::u32string GetAlphabetInRange(char32_t from, char32_t to);
    static std::u32string GetAlphabetFromText(const std::string& text);
    static std::u32string GetAlphabetFromText(const std::u32string& text);

    ~Font();

    Ptr<Font>               GetPtr() { return shared_from_this(); }
    const Settings&         GetSettings() const { return m_settings; }

    void                    ResetChars(const std::string& utf8_characters);
    void                    ResetChars(const std::u32string& utf32_characters);
    void                    AddChars(const std::string& utf8_characters);
    void                    AddChars(const std::u32string& utf32_characters);
    const Font::Char&       AddChar(Char::Code char_code);
    bool                    HasChar(Char::Code char_code);
    const Char&             GetChar(Char::Code char_code) const;
    Chars                   GetChars() const;
    Chars                   GetTextChars(const std::string& text);
    Chars                   GetTextChars(const std::u32string& text);
    gfx:: FrameRect::Point  GetKerning(const Char& left_char, const Char& right_char) const;
    uint32_t                GetLineHeight() const noexcept;
    const gfx::FrameSize&   GetAtlasSize() const noexcept;
    const Ptr<gfx::Texture>& GetAtlasTexturePtr(gfx::Context& context);
    gfx::Texture&           GetAtlasTexture(gfx::Context& context) { return *GetAtlasTexturePtr(context); }

protected:
    // Font can be created only via Font::Library::Add
    Font(const Data::Provider& data_provider, const Settings& settings);

    Refs<Char> GetMutableChars();
    bool PackCharsToAtlas(float pixels_reserve_multiplier);

    // IContextCallback interface
    void OnContextReleased(gfx::Context& context) override;
    void OnContextCompletingInitialization(gfx::Context& context) override;
    void OnContextInitialized(gfx::Context&) override { }

private:
    struct AtlasTexture
    {
        Ptr<gfx::Texture> sp_texture;
        bool              is_update_required = true;
    };

    AtlasTexture CreateAtlasTexture(gfx::Context& context, bool deferred_data_init);
    void RemoveAtlasTexture(gfx::Context& context);

    bool UpdateAtlasBitmap(bool deferred_textures_update);
    void UpdateAtlasTextures(bool deferred_textures_update);
    void UpdateAtlasTexture(gfx::Context& context, AtlasTexture& atlas_texture);
    void ClearAtlasTextures();

    class Face;
    using TextureByContext = std::map<gfx::Context*, AtlasTexture>;
    using CharByCode = std::map<Char::Code, Char>;

    Settings               m_settings;
    UniquePtr<Face>        m_sp_face;
    UniquePtr<CharBinPack> m_sp_atlas_pack;
    CharByCode             m_char_by_code;
    Data::Bytes            m_atlas_bitmap;
    TextureByContext       m_atlas_textures;
};

} // namespace Methane::Graphics
