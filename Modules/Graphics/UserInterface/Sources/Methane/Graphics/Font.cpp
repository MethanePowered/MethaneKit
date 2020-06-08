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

FILE: Methane/Graphics/Font.cpp
Font atlas textures generation and fonts library management classes.

******************************************************************************/

#include <Methane/Graphics/Font.h>

#include <Methane/Data/RectBinPack.hpp>
#include <Methane/Instrumentation.h>

#include <nowide/convert.hpp>
#include <map>
#include <set>
#include <algorithm>
#include <cassert>

extern "C"
{
#include <ft2build.h>
#include <freetype/ftglyph.h>
#include FT_FREETYPE_H
}

namespace Methane::Graphics
{

static const char* GetFTErrorMessage(FT_Error err)
{
    META_FUNCTION_TASK();

#undef __FTERRORS_H__
#define FT_ERRORDEF( e, v, s )  case e: return s;
#define FT_ERROR_START_LIST     switch (err) {
#define FT_ERROR_END_LIST       }
#include FT_ERRORS_H

    return "(Unknown error)";
}

inline void ThrowFreeTypeError(FT_Error error)
{
    if (error)
    {
        const std::string error_msg = "Unexpected free type error occurred: ";
        throw std::runtime_error(error_msg + GetFTErrorMessage(error));
    }
}

class Font::Library::Impl
{
public:
    Impl()
    {
        META_FUNCTION_TASK();
        ThrowFreeTypeError(FT_Init_FreeType(&m_ft_library));
    }

    ~Impl()
    {
        META_FUNCTION_TASK();
        FT_Done_FreeType(m_ft_library);
    }

    FT_Library GetFTLib() const { return m_ft_library; }

private:
    FT_Library m_ft_library;
};

class Font::Char::Glyph
{
public:
    Glyph(FT_Glyph ft_glyph, uint32_t face_index)
        : m_ft_glyph(ft_glyph)
        , m_face_index(face_index)
    {
        META_FUNCTION_TASK();
    }

    ~Glyph()
    {
        META_FUNCTION_TASK();
        FT_Done_Glyph(m_ft_glyph);
    }

    FT_Glyph GetFTGlyph() const   { return m_ft_glyph; }
    uint32_t GetFaceIndex() const { return m_face_index; }

private:
    const FT_Glyph m_ft_glyph;
    const uint32_t m_face_index;
};

class Font::Face
{
public:
    Face(Data::Chunk&& font_data)
        : m_font_data(std::move(font_data))
        , m_ft_face(LoadFace(Library::Get().GetImpl().GetFTLib(), m_font_data))
        , m_has_kerning(static_cast<bool>(FT_HAS_KERNING(m_ft_face)))
    {
        META_FUNCTION_TASK();
    }

    ~Face()
    {
        META_FUNCTION_TASK();
        FT_Done_Face(m_ft_face);
    }

    void SetSize(uint32_t font_size_pt, uint32_t resolution_dpi)
    {
        META_FUNCTION_TASK();
        // font Size is measured in 1/64ths of pixels
        // 0 values mean that vertical value is equal horizontal value
        ThrowFreeTypeError(FT_Set_Char_Size(m_ft_face, font_size_pt * 64, 0, resolution_dpi, 0));
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
        if (!char_index)
        {
            const std::string error_message = "Character " + std::to_string(char_code) + " does not exist in font face.";
            throw std::runtime_error(error_message);
        }

        ThrowFreeTypeError(FT_Load_Glyph(m_ft_face, char_index, FT_LOAD_RENDER));

        FT_Glyph ft_glyph = nullptr;
        ThrowFreeTypeError(FT_Get_Glyph(m_ft_face->glyph, &ft_glyph));

        // All glyph metrics are multiplied by 64, so we reverse them back
        return Char(char_code,
            {
                Point2i(),
                FrameSize(static_cast<uint32_t>(m_ft_face->glyph->metrics.width >> 6),
                          static_cast<uint32_t>(m_ft_face->glyph->metrics.height >> 6))
            },
            Point2i(static_cast<int32_t>(m_ft_face->glyph->metrics.horiBearingX >> 6),
                    -static_cast<int32_t>(m_ft_face->glyph->metrics.horiBearingY >> 6)),
            Point2i(static_cast<int32_t>(m_ft_face->glyph->metrics.horiAdvance >> 6),
                    static_cast<int32_t>(m_ft_face->glyph->metrics.vertAdvance >> 6)),
            std::make_unique<Char::Glyph>(ft_glyph, char_index)
        );
    }

    FrameRect::Point GetKerning(uint32_t left_glyph_index, uint32_t right_glyph_index) const
    {
        META_FUNCTION_TASK();
        assert(left_glyph_index && right_glyph_index);
        if (!m_has_kerning || !left_glyph_index || !right_glyph_index)
            return FrameRect::Point(0, 0);

        FT_Vector kerning_vec{};
        ThrowFreeTypeError(FT_Get_Kerning(m_ft_face, left_glyph_index, right_glyph_index, FT_KERNING_DEFAULT, &kerning_vec));
        return FrameRect::Point(kerning_vec.x >> 6, 0);
    }

private:
    static FT_Face LoadFace(FT_Library ft_library, const Data::Chunk& font_data)
    {
        META_FUNCTION_TASK();

        FT_Face ft_face = nullptr;
        ThrowFreeTypeError(FT_New_Memory_Face(ft_library,
            static_cast<const FT_Byte*>(font_data.p_data),
            static_cast<FT_Long>(font_data.size), 0,
            &ft_face));

        return ft_face;
    }

    const Data::Chunk m_font_data;
    const FT_Face     m_ft_face = nullptr;
    const bool        m_has_kerning;
};

class Font::CharBinPack : public Data::RectBinPack<FrameRect>
{
public:
    using FrameBinPack = Data::RectBinPack<FrameRect>;
    using FrameBinPack::RectBinPack;

    bool TryPack(const Refs<Font::Char>& font_chars)
    {
        META_FUNCTION_TASK();
        for(const Ref<Font::Char>& font_char : font_chars)
        {
            if (!TryPack(font_char.get()))
                return false;
        }
        return true;
    }

    bool TryPack(Font::Char& font_char)
    {
        return FrameBinPack::TryPack(font_char.rect);
    }
};

Font::Library& Font::Library::Get()
{
    META_FUNCTION_TASK();
    static Library s_library;
    return s_library;
}

Refs<Font> Font::Library::GetFonts() const
{
    META_FUNCTION_TASK();
    Refs<Font> font_refs;
    for(const auto& name_and_font_ptr : m_font_by_name)
    {
        font_refs.emplace_back(*name_and_font_ptr.second);
    }
    return font_refs;
}

bool Font::Library::HasFont(const std::string& font_name) const
{
    META_FUNCTION_TASK();
    return m_font_by_name.count(font_name);
}

Font& Font::Library::GetFont(const std::string& font_name) const
{
    META_FUNCTION_TASK();
    const auto font_by_name_it = m_font_by_name.find(font_name);
    if (font_by_name_it == m_font_by_name.end())
        throw std::invalid_argument("There is no font with name \"" + font_name + "\" in library.");

    assert(font_by_name_it->second);
    return *font_by_name_it->second;
}

Font& Font::Library::GetFont(const Data::Provider& data_provider, const Settings& font_settings)
{
    const auto font_by_name_it = m_font_by_name.find(font_settings.name);
    if (font_by_name_it != m_font_by_name.end())
    {
        assert(font_by_name_it->second);
        return *font_by_name_it->second;
    }
    return AddFont(data_provider, font_settings);
}

Font& Font::Library::AddFont(const Data::Provider& data_provider, const Settings& font_settings)
{
    META_FUNCTION_TASK();
    auto emplace_result = m_font_by_name.emplace(font_settings.name, Ptr<Font>(new Font(data_provider, font_settings)));
    if (!emplace_result.second)
        throw std::invalid_argument("Font with name \"" + font_settings.name + "\" already exists in library.");

    assert(emplace_result.first->second);
    return *emplace_result.first->second;
}

void Font::Library::RemoveFont(const std::string& font_name)
{
    META_FUNCTION_TASK();
    const auto font_by_name_it = m_font_by_name.find(font_name);
    if (font_by_name_it != m_font_by_name.end())
        m_font_by_name.erase(font_by_name_it);
}

void Font::Library::Clear()
{
    META_FUNCTION_TASK();
    m_font_by_name.clear();
}

Font::Library::Library()
    : m_sp_impl(std::make_unique<Impl>())
{
    META_FUNCTION_TASK();
}

std::string Font::GetAnsiCharacters(const char from, const char to)
{
    META_FUNCTION_TASK();
    if (from > to)
        throw std::invalid_argument("Invalid characters range provided [" + std::to_string(from) + ", " + std::to_string(to) + "]");

    std::string characters(to - from + 1, 0);
    for(char curr = from; curr <= to; ++curr)
    {
        characters[curr - from] = curr;
    }
    return characters;
}

std::string Font::GetTextAlphabet(const std::string& text)
{
    META_FUNCTION_TASK();
    std::wstring wide_text = nowide::widen(text);

    std::set<wchar_t> alphabet;
    for(wchar_t wide_char : wide_text)
    {
        alphabet.insert(wide_char);
    }

    std::wstringstream alphabet_ss;
    for(wchar_t wide_char : alphabet)
    {
        alphabet_ss << wide_char;
    }
    return nowide::narrow(alphabet_ss.str());
}

Font::Font(const Data::Provider& data_provider, const Settings& settings)
    : m_settings(settings)
    , m_sp_face(std::make_unique<Face>(data_provider.GetData(m_settings.font_path)))
{
    META_FUNCTION_TASK();

    m_sp_face->SetSize(m_settings.font_size_pt, m_settings.resolution_dpi);
    AddChars(m_settings.characters);
}

Font::~Font()
{
    META_FUNCTION_TASK();
    ClearAtlasTextures();
}

void Font::ResetChars(const std::string& unicode_characters)
{
    META_FUNCTION_TASK();
    ResetChars(nowide::widen(unicode_characters));
}

void Font::ResetChars(const std::wstring& characters)
{
    META_FUNCTION_TASK();
    m_sp_atlas_pack.reset();
    m_char_by_code.clear();
    m_atlas_bitmap.clear();
    m_max_glyph_size = FrameSize();

    AddChars(characters);
    PackCharsToAtlas(1.2f);
    UpdateAtlasBitmap();
}

void Font::AddChars(const std::string& unicode_characters)
{
    META_FUNCTION_TASK();
    AddChars(nowide::widen(unicode_characters));
}

void Font::AddChars(const std::wstring& characters)
{
    META_FUNCTION_TASK();
    for (wchar_t character : characters)
    {
        AddChar(static_cast<Char::Code>(character));
    }
}

const Font::Char& Font::AddChar(Char::Code char_code)
{
    META_FUNCTION_TASK();
    const Char& font_char = GetChar(char_code);
    if (font_char)
        return font_char;

    // Load char glyph and add it to the font characters map
    auto font_char_it = m_char_by_code.emplace(char_code, m_sp_face->LoadChar(char_code)).first;
    assert(font_char_it != m_char_by_code.end());

    Char& new_font_char = font_char_it->second;
    if (!m_sp_atlas_pack)
        return new_font_char;

    // Attempt to pack new char into existing atlas
    if (m_sp_atlas_pack->TryPack(new_font_char))
    {
        // Draw char to existing atlas bitmap and update textures;
        new_font_char.DrawToAtlas(m_atlas_bitmap, m_sp_atlas_pack->GetSize().width);
        UpdateAtlasTextures();
        return new_font_char;
    }

    // If new char does not fit into existing atlas, repack all chars into new atlas
    PackCharsToAtlas(2.f);
    UpdateAtlasBitmap();

    return new_font_char;
}

bool Font::HasChar(Char::Code char_code)
{
    META_FUNCTION_TASK();
    return m_char_by_code.count(char_code);
}

const Font::Char& Font::GetChar(Char::Code char_code) const
{
    META_FUNCTION_TASK();
    static const Char s_none_char {};
    static const Char s_line_break(static_cast<Char::Code>('\n'));
    if (char_code == s_line_break.code)
        return s_line_break;

    const auto char_by_code_it = m_char_by_code.find(char_code);
    return char_by_code_it == m_char_by_code.end() ? s_none_char : char_by_code_it->second;
}

Refs<const Font::Char> Font::GetChars() const
{
    META_FUNCTION_TASK();
    Refs<const Char> font_chars;
    for(const auto& code_and_char : m_char_by_code)
    {
        font_chars.emplace_back(code_and_char.second);
    }
    return font_chars;
}

Refs<const Font::Char> Font::GetTextChars(const std::string& text)
{
    META_FUNCTION_TASK();
    Refs<const Char> font_chars;
    const std::wstring characters = nowide::widen(text);
    for (wchar_t char_code : characters)
    {
        // TODO: do not update atlast textures on adding every new character to atlas, make deferred atlas updates
        font_chars.emplace_back(AddChar(static_cast<Char::Code>(char_code)));
    }
    return font_chars;
}

FrameRect::Point Font::GetKerning(const Char& left_char, const Char& right_char) const
{
    META_FUNCTION_TASK();
    assert(!!m_sp_face);
    return m_sp_face->GetKerning(left_char.GetGlyphIndex(), right_char.GetGlyphIndex());
}

Refs<Font::Char> Font::GetMutableChars()
{
    META_FUNCTION_TASK();
    Refs<Char> font_chars;
    for(auto& code_and_char : m_char_by_code)
    {
        font_chars.emplace_back(code_and_char.second);
    }
    return font_chars;
}

bool Font::PackCharsToAtlas(float pixels_reserve_multiplier)
{
    META_FUNCTION_TASK();

    // Transform char-map to vector of chars
    Refs<Char> font_chars = GetMutableChars();
    if (font_chars.empty())
        return false;

    // Sort chars by decreasing of glyph pixels count from largest to smallest
    std::sort(font_chars.begin(), font_chars.end(),
        [](const Ref<Char>& left, const Ref<Char>& right) -> bool
        { return left.get() > right.get(); }
    );
    m_max_glyph_size = font_chars.front().get().rect.size;

    // Estimate required atlas size
    uint32_t char_pixels_count = 0u;
    for(Font::Char& font_char : font_chars)
    {
        char_pixels_count += font_char.rect.size.GetPixelsCount();
    }
    char_pixels_count = static_cast<uint32_t>(char_pixels_count * pixels_reserve_multiplier);
    const uint32_t square_atlas_dimension = static_cast<uint32_t>(std::sqrt(char_pixels_count));

    // Pack all character glyphs intro atlas size with doubling the size until all chars fit in
    FrameSize atlas_size(square_atlas_dimension, square_atlas_dimension);
    m_sp_atlas_pack = std::make_unique<CharBinPack>(atlas_size);
    while(!m_sp_atlas_pack->TryPack(font_chars))
    {
        atlas_size *= 2;
        m_sp_atlas_pack = std::make_unique<CharBinPack>(atlas_size);
    }
    return true;
}

const Ptr<Texture>& Font::GetAtlasTexturePtr(Context& context)
{
    META_FUNCTION_TASK();
    const auto atlas_texture_it = m_atlas_textures.find(&context);
    if (atlas_texture_it != m_atlas_textures.end())
    {
        assert(!!atlas_texture_it->second);
        return atlas_texture_it->second;
    }

    static const Ptr<Texture> sp_empty_texture;
    assert(!m_char_by_code.empty());
    if (m_char_by_code.empty())
        return sp_empty_texture;

    if (!m_sp_atlas_pack)
    {
        // Reserve 20% of pixels for packing space loss and for adding new characters to atlas
        if (!PackCharsToAtlas(1.2f))
            return sp_empty_texture;
    }

    // Add font as context callback to remove atlas texture when context is released
    context.Connect(*this);

    // Create atlas texture and render glyphs to it
    UpdateAtlasBitmap();
    return m_atlas_textures.emplace(&context, CreateAtlasTexture(context)).first->second;
}

Ptr<Texture> Font::CreateAtlasTexture(Context& context)
{
    META_FUNCTION_TASK();
    Ptr<Texture> sp_atlas_texture = Texture::CreateImage(context, Dimensions(m_sp_atlas_pack->GetSize()), 1, PixelFormat::R8Unorm, false);
    sp_atlas_texture->SetName(m_settings.name + " Font Atlas");
    sp_atlas_texture->SetData({
        Resource::SubResource(reinterpret_cast<Data::ConstRawPtr>(m_atlas_bitmap.data()), static_cast<Data::Size>(m_atlas_bitmap.size()))
    });
    return sp_atlas_texture;
}

void Font::RemoveAtlasTexture(Context& context)
{
    META_FUNCTION_TASK();
    m_atlas_textures.erase(&context);
    context.Disconnect(*this);
}

bool Font::UpdateAtlasBitmap()
{
    META_FUNCTION_TASK();
    if (!m_sp_atlas_pack)
        throw std::logic_error("Can not update atlas bitmap until atlas is packed.");

    const FrameSize atlas_size = m_sp_atlas_pack->GetSize();
    if (m_atlas_bitmap.size() == atlas_size.GetPixelsCount())
        return false;

    // Clear old atlas content
    std::fill(m_atlas_bitmap.begin(), m_atlas_bitmap.end(), uint8_t(0));
    m_atlas_bitmap.resize(atlas_size.GetPixelsCount(), uint8_t(0));

    // Render glyphs to atlas bitmap
    for (const auto& code_and_char : m_char_by_code)
    {
        code_and_char.second.DrawToAtlas(m_atlas_bitmap, atlas_size.width);
    }

    UpdateAtlasTextures();
    return true;
}

void Font::UpdateAtlasTextures()
{
    META_FUNCTION_TASK();
    if (!m_sp_atlas_pack)
        throw std::logic_error("Can not update atlas textures until atlas is packed and bitmap is up to date.");

    const FrameSize atlas_size = m_sp_atlas_pack->GetSize();
    for(auto& context_and_texture : m_atlas_textures)
    {
        assert(context_and_texture.first);
        assert(context_and_texture.second);
        Context&      context    = *context_and_texture.first;
        Ptr<Texture>& sp_texture = context_and_texture.second;

        const Dimensions& texture_dimensions = sp_texture->GetSettings().dimensions;
        if (texture_dimensions.width != atlas_size.width || texture_dimensions.height != atlas_size.height)
        {
            const Ptr<Texture> sp_old_texture = sp_texture;
            sp_texture = CreateAtlasTexture(context);
            Emit(&IFontCallback::OnFontAtlasTextureReset, *this, sp_old_texture, sp_texture);
        }
        else
        {
            // TODO: Update only a region of atlas texture containing character bitmap
            sp_texture->SetData({
                Resource::SubResource(reinterpret_cast<Data::ConstRawPtr>(m_atlas_bitmap.data()), static_cast<Data::Size>(m_atlas_bitmap.size()))
            });
        }
    }
}

void Font::ClearAtlasTextures()
{
    META_FUNCTION_TASK();
    for(const auto& context_and_texture : m_atlas_textures)
    {
        if (context_and_texture.first)
            context_and_texture.first->Disconnect(*this);
    }
    m_atlas_textures.clear();
}

void Font::OnContextReleased(Context& context)
{
    META_FUNCTION_TASK();
    RemoveAtlasTexture(context);
}

Font::Char::Char(Code code)
    : code(code)
{
    META_FUNCTION_TASK();
}

Font::Char::Char(Code code, FrameRect rect, Point2i offset, Point2i advance, UniquePtr<Glyph>&& sp_glyph)
    : code(code)
    , rect(std::move(rect))
    , offset(std::move(offset))
    , advance(std::move(advance))
    , m_sp_glyph(std::move(sp_glyph))
{
    META_FUNCTION_TASK();
}

void Font::Char::DrawToAtlas(Data::Bytes& atlas_bitmap, uint32_t atlas_row_stride) const
{
    META_FUNCTION_TASK();
    if (!rect.size)
        return; 

    // Verify glyph placement
    assert(rect.GetLeft() >= 0 && static_cast<uint32_t>(rect.GetRight())  <= atlas_row_stride);
    assert(rect.GetTop()  >= 0 && static_cast<uint32_t>(rect.GetBottom()) <= static_cast<uint32_t>(atlas_bitmap.size() / atlas_row_stride));

    // Draw glyph to bitmap
    FT_Glyph ft_glyph = m_sp_glyph->GetFTGlyph();
    ThrowFreeTypeError(FT_Glyph_To_Bitmap(&ft_glyph, FT_RENDER_MODE_NORMAL, nullptr, false));

    FT_Bitmap& ft_bitmap = reinterpret_cast<FT_BitmapGlyph>(ft_glyph)->bitmap;
    assert(ft_bitmap.width == rect.size.width);
    assert(ft_bitmap.rows == rect.size.height);

    // Copy glyph pixels to output bitmap row-by-row
    for (uint32_t y = 0; y < ft_bitmap.rows; y++)
    {
        const uint32_t atlas_index = rect.origin.GetX() + (rect.origin.GetY() + y) * atlas_row_stride;
        if (atlas_index + ft_bitmap.width > atlas_bitmap.size())
            throw std::runtime_error("Char glyph does not fit into target atlas bitmap.");

        std::copy(ft_bitmap.buffer + y * ft_bitmap.width, ft_bitmap.buffer + (y + 1) * ft_bitmap.width, atlas_bitmap.begin() + atlas_index);
    }
}

uint32_t Font::Char::GetGlyphIndex() const
{
    META_FUNCTION_TASK();
    assert(!!m_sp_glyph);
    return m_sp_glyph->GetFaceIndex();
}

} // namespace Methane::Graphics
