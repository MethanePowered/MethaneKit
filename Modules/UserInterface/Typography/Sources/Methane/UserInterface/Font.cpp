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

FILE: Methane/UserInterface/Font.cpp
Font atlas textures generation and fonts library management classes.

******************************************************************************/

#include <Methane/UserInterface/Font.h>

#include <Methane/Graphics/RenderContext.h>
#include <Methane/Graphics/Texture.h>
#include <Methane/Data/RectBinPack.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <nowide/convert.hpp>
#include <magic_enum.hpp>
#include <map>
#include <set>
#include <locale>
#include <codecvt>
#include <algorithm>

#include <ft2build.h>
#include <freetype/ftglyph.h>
#include FT_FREETYPE_H

static constexpr int32_t g_ft_dots_in_pixel = 64; // Freetype measures all font sizes in 1/64ths of pixels

namespace Methane::UserInterface
{

[[nodiscard]] static const char* GetFTErrorMessage(FT_Error err)
{
    META_FUNCTION_TASK();

#undef __FTERRORS_H__ //NOSONARs
#define FT_ERRORDEF( e, v, s )  case e: return s;
#define FT_ERROR_START_LIST     switch (err) {
#define FT_ERROR_END_LIST       }
#include FT_ERRORS_H  //NOSONAR

    return "(Unknown error)";
}

inline void ThrowFreeTypeError(FT_Error error)
{
    if (error)
        throw Font::FreeTypeError(error);
}

Font::FreeTypeError::FreeTypeError(FT_Error error)
    : std::runtime_error(fmt::format("Unexpected FreeType error occurred '{}'", GetFTErrorMessage(error)))
    , m_error(error)
{
    META_FUNCTION_TASK();
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

    Impl(const Impl&) noexcept = delete;
    Impl(Impl&&) noexcept = delete;

    Impl& operator=(const Impl&) noexcept = delete;
    Impl& operator=(Impl&&) noexcept = delete;

    [[nodiscard]] FT_Library GetFTLib() const { return m_ft_library; }

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

    Glyph(const Glyph&) noexcept = delete;
    Glyph(Glyph&&) noexcept = delete;

    Glyph& operator=(const Glyph&) noexcept = delete;
    Glyph& operator=(Glyph&&) noexcept = delete;

    [[nodiscard]] FT_Glyph GetFTGlyph() const   { return m_ft_glyph; }
    [[nodiscard]] uint32_t GetFaceIndex() const { return m_face_index; }

private:
    const FT_Glyph m_ft_glyph;
    const uint32_t m_face_index;
};

class Font::Face
{
public:
    explicit Face(Data::Chunk&& font_data)
        : m_font_data(std::move(font_data))
        , m_ft_face(LoadFace(Library::Get().GetImpl().GetFTLib(), m_font_data))
        , m_ft_face_rec(GetFaceRec())
        , m_has_kerning(static_cast<bool>(FT_HAS_KERNING(m_ft_face)))
    {
        META_FUNCTION_TASK();
    }

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
        ThrowFreeTypeError(FT_Set_Char_Size(m_ft_face, font_size_pt * g_ft_dots_in_pixel, 0, resolution_dpi, 0));
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
        META_CHECK_ARG_NOT_ZERO_DESCR(char_index, "unicode character U+{} does not exist in font face", static_cast<uint32_t>(char_code));

        ThrowFreeTypeError(FT_Load_Glyph(m_ft_face, char_index, FT_LOAD_RENDER));
        META_CHECK_ARG_NOT_NULL_DESCR(m_ft_face_rec.glyph, "glyph should not be null after loading from font face");

        FT_Glyph ft_glyph = nullptr;
        ThrowFreeTypeError(FT_Get_Glyph(m_ft_face_rec.glyph, &ft_glyph));

        // All glyph metrics are multiplied by 64, so we reverse them back
        return Char(char_code,
            {
                gfx::Point2I(),
                gfx::FrameSize(static_cast<uint32_t>(m_ft_face_rec.glyph->metrics.width  / g_ft_dots_in_pixel),
                               static_cast<uint32_t>(m_ft_face_rec.glyph->metrics.height / g_ft_dots_in_pixel))
            },
            gfx::Point2I(static_cast<int32_t>(m_ft_face_rec.glyph->metrics.horiBearingX  / g_ft_dots_in_pixel),
                        -static_cast<int32_t>(m_ft_face_rec.glyph->metrics.horiBearingY  / g_ft_dots_in_pixel)),
            gfx::Point2I(static_cast<int32_t>(m_ft_face_rec.glyph->metrics.horiAdvance   / g_ft_dots_in_pixel),
                         static_cast<int32_t>(m_ft_face_rec.glyph->metrics.vertAdvance   / g_ft_dots_in_pixel)),
            std::make_unique<Char::Glyph>(ft_glyph, char_index)
        );
    }

    gfx::FramePoint GetKerning(uint32_t left_glyph_index, uint32_t right_glyph_index) const
    {
        META_FUNCTION_TASK();
        if (!m_has_kerning)
            return gfx::FramePoint(0, 0);

        META_CHECK_ARG_NOT_ZERO(left_glyph_index);
        META_CHECK_ARG_NOT_ZERO(right_glyph_index);

        FT_Vector kerning_vec{};
        ThrowFreeTypeError(FT_Get_Kerning(m_ft_face, left_glyph_index, right_glyph_index, FT_KERNING_DEFAULT, &kerning_vec));
        return gfx::FramePoint(static_cast<int>(kerning_vec.x >> 6), 0);
    }

    uint32_t GetLineHeight() const
    {
        META_FUNCTION_TASK();
        META_CHECK_ARG_NOT_NULL(m_ft_face_rec.size);
        return static_cast<uint32_t>(m_ft_face_rec.size->metrics.height / g_ft_dots_in_pixel);
    }

    const FT_FaceRec& GetFaceRec() const
    {
        META_FUNCTION_TASK();
        META_CHECK_ARG_NOT_NULL(m_ft_face);
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

class Font::CharBinPack : public Data::RectBinPack<gfx::FrameRect>
{
public:
    using FrameBinPack = Data::RectBinPack<gfx::FrameRect>;
    using FrameBinPack::RectBinPack;

    bool TryPack(const Refs<Font::Char>& font_chars)
    {
        META_FUNCTION_TASK();
        return std::all_of(font_chars.begin(), font_chars.end(),
            [this](const Ref<Font::Char>& font_char)
            {
                return TryPack(font_char.get());
            }
        );
    }

    bool TryPack(Font::Char& font_char)
    {
        return FrameBinPack::TryPack(font_char.m_rect);
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
    for(const auto& [font_name, font_ptr] : m_font_by_name)
    {
        font_refs.emplace_back(*font_ptr);
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
    META_CHECK_ARG_DESCR(font_name, font_by_name_it != m_font_by_name.end(), "there is no font with a give name in fonts library");
    META_CHECK_ARG_NOT_NULL(font_by_name_it->second);

    return *font_by_name_it->second;
}

Font& Font::Library::GetFont(const Data::Provider& data_provider, const Settings& font_settings)
{
    META_FUNCTION_TASK();
    const auto font_by_name_it = m_font_by_name.find(font_settings.description.name);
    if (font_by_name_it == m_font_by_name.end())
        return AddFont(data_provider, font_settings);

    META_CHECK_ARG_NOT_NULL(font_by_name_it->second);
    return *font_by_name_it->second;
}

Font& Font::Library::AddFont(const Data::Provider& data_provider, const Settings& font_settings)
{
    META_FUNCTION_TASK();
    auto [ name_and_font_it, font_added ] = m_font_by_name.try_emplace(font_settings.description.name, new Font(data_provider, font_settings)); // NOSONAR
    META_CHECK_ARG_DESCR(font_settings.description.name, font_added, "font with a give name already exists in fonts library");

    META_CHECK_ARG_NAME("emplace_result", name_and_font_it->second);
    Emit(&IFontLibraryCallback::OnFontAdded, *name_and_font_it->second);

    return *name_and_font_it->second;
}

void Font::Library::RemoveFont(const std::string& font_name)
{
    META_FUNCTION_TASK();
    const auto font_by_name_it = m_font_by_name.find(font_name);
    if (font_by_name_it == m_font_by_name.end())
        return;

    Emit(&IFontLibraryCallback::OnFontRemoved, *font_by_name_it->second);
    m_font_by_name.erase(font_by_name_it);
}

void Font::Library::Clear()
{
    META_FUNCTION_TASK();
    m_font_by_name.clear();
}

Font::Library::Library()
    : m_impl_ptr(std::make_unique<Impl>())
{
    META_FUNCTION_TASK();
}

std::u32string Font::ConvertUtf8To32(std::string_view text)
{
    META_FUNCTION_TASK();
    static std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
    return converter.from_bytes(text.data(), text.data() + text.length());
}

std::string Font::ConvertUtf32To8(std::u32string_view text)
{
    META_FUNCTION_TASK();
    static std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
    return converter.to_bytes(text.data(), text.data() + text.length());
}

std::u32string Font::GetAlphabetInRange(char32_t from, char32_t to)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(static_cast<uint32_t>(to), static_cast<uint32_t>(from), "invalid characters range");

    std::u32string alphabet(to - from + 1, 0);
    for(char32_t utf32_char = from; utf32_char <= to; ++utf32_char)
    {
        alphabet[utf32_char - from] = utf32_char;
    }
    return alphabet;
}

std::u32string Font::GetAlphabetFromText(const std::string& text)
{
    META_FUNCTION_TASK();
    return GetAlphabetFromText(ConvertUtf8To32(text));
}

std::u32string Font::GetAlphabetFromText(const std::u32string& utf32_text)
{
    META_FUNCTION_TASK();

    std::set<char32_t> alphabet_set;
    for(char32_t utf32_char : utf32_text)
    {
        alphabet_set.insert(utf32_char);
    }

    std::u32string alphabet(alphabet_set.size() + 1, 0);
    size_t alpha_index = 0;
    for(char32_t utf32_char : alphabet_set)
    {
        alphabet[alpha_index] = utf32_char;
        alpha_index++;
    }

    return alphabet;
}

Font::Font(const Data::Provider& data_provider, const Settings& settings)
    : m_settings(settings)
    , m_face_ptr(std::make_unique<Face>(data_provider.GetData(m_settings.description.path)))
{
    META_FUNCTION_TASK();

    m_face_ptr->SetSize(m_settings.description.size_pt, m_settings.resolution_dpi);
    AddChars(m_settings.characters);
}

Font::~Font()
{
    META_FUNCTION_TASK();
    ClearAtlasTextures();
}

void Font::ResetChars(const std::string& utf8_characters)
{
    META_FUNCTION_TASK();
    ResetChars(ConvertUtf8To32(utf8_characters));
}

void Font::ResetChars(const std::u32string& utf32_characters)
{
    META_FUNCTION_TASK();
    m_atlas_pack_ptr.reset();
    m_char_by_code.clear();
    m_atlas_bitmap.clear();

    if (utf32_characters.empty())
    {
        for(const auto& [context_ptr, atlas_texture] : m_atlas_textures)
        {
            Emit(&IFontCallback::OnFontAtlasTextureReset, *this, atlas_texture.texture_ptr, nullptr);
        }
        m_atlas_textures.clear();
        return;
    }

    AddChars(utf32_characters);
    PackCharsToAtlas(1.2F);
    UpdateAtlasBitmap(false);
}

void Font::AddChars(const std::string& utf8_characters)
{
    META_FUNCTION_TASK();
    ResetChars(ConvertUtf8To32(utf8_characters));
}

void Font::AddChars(const std::u32string& utf32_characters)
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

const Font::Char& Font::AddChar(Char::Code char_code)
{
    META_FUNCTION_TASK();
    if (const Char& font_char = GetChar(char_code); font_char)
        return font_char;

    // Load char glyph and add it to the font characters map
    const auto font_char_it = m_char_by_code.try_emplace(char_code, m_face_ptr->LoadChar(char_code)).first;
    META_CHECK_ARG_DESCR(static_cast<uint32_t>(char_code), font_char_it != m_char_by_code.end(), "font character was not added to character map");

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

bool Font::HasChar(Char::Code char_code) const
{
    META_FUNCTION_TASK();
    return m_char_by_code.count(char_code) ||
           char_code == static_cast<Char::Code>('\n');
}

const Font::Char& Font::GetChar(Char::Code char_code) const
{
    META_FUNCTION_TASK();
    static const Char s_none_char {};
    if (static const Char s_line_break(static_cast<Char::Code>('\n'));
        char_code == s_line_break.GetCode())
        return s_line_break;

    const auto char_by_code_it = m_char_by_code.find(char_code);
    return char_by_code_it == m_char_by_code.end() ? s_none_char : char_by_code_it->second;
}

Font::Chars Font::GetChars() const
{
    META_FUNCTION_TASK();
    Chars font_chars;
    for(const auto& [char_code, character] : m_char_by_code)
    {
        font_chars.emplace_back(character);
    }
    return font_chars;
}

Font::Chars Font::GetTextChars(const std::string& text)
{
    META_FUNCTION_TASK();
    return GetTextChars(ConvertUtf8To32(text));
}

Font::Chars Font::GetTextChars(const std::u32string& text)
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

gfx::FramePoint Font::GetKerning(const Char& left_char, const Char& right_char) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_face_ptr);
    return m_face_ptr->GetKerning(left_char.GetGlyphIndex(), right_char.GetGlyphIndex());
}

uint32_t Font::GetLineHeight() const
{
    META_FUNCTION_TASK();
    return m_face_ptr->GetLineHeight();
}

const gfx::FrameSize& Font::GetAtlasSize() const noexcept
{
    META_FUNCTION_TASK();
    static const gfx::FrameSize s_empty_size;
    return m_atlas_pack_ptr ? m_atlas_pack_ptr->GetSize() : s_empty_size;
}

Refs<Font::Char> Font::GetMutableChars()
{
    META_FUNCTION_TASK();
    Refs<Char> font_chars;
    for(auto& [char_code, character] : m_char_by_code)
    {
        font_chars.emplace_back(character);
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
        [](const Ref<Char>& left, const Ref<Char>& right)
        { return left.get() > right.get(); }
    );

    // Estimate required atlas size
    uint32_t char_pixels_count = 0U;
    for(const Font::Char& font_char : font_chars)
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

const Ptr<gfx::Texture>& Font::GetAtlasTexturePtr(gfx::Context& context)
{
    META_FUNCTION_TASK();
    if (const auto atlas_texture_it = m_atlas_textures.find(&context);
        atlas_texture_it != m_atlas_textures.end())
    {
        META_CHECK_ARG_NOT_NULL(atlas_texture_it->second.texture_ptr);
        return atlas_texture_it->second.texture_ptr;
    }

    static const Ptr<gfx::Texture> empty_texture_ptr;
    if (m_char_by_code.empty())
        return empty_texture_ptr;

    // Reserve 20% of pixels for packing space loss and for adding new characters to atlas
    if (!m_atlas_pack_ptr && !PackCharsToAtlas(1.2F))
        return empty_texture_ptr;

    // Add font as context callback to remove atlas texture when context is released
    context.Connect(*this);

    // Create atlas texture and render glyphs to it
    UpdateAtlasBitmap(true);

    const Ptr<gfx::Texture>& atlas_texture_ptr = m_atlas_textures.try_emplace(&context, CreateAtlasTexture(context, true)).first->second.texture_ptr;
    Emit(&IFontCallback::OnFontAtlasTextureReset, *this, nullptr, atlas_texture_ptr);

    return atlas_texture_ptr;
}

gfx::Texture& Font::GetAtlasTexture(gfx::Context& context)
{
    const Ptr<gfx::Texture>& texture_ptr = GetAtlasTexturePtr(context);
    META_CHECK_ARG_NOT_NULL_DESCR(texture_ptr, "atlas texture is not available for context");
    return *texture_ptr;
}

Font::AtlasTexture Font::CreateAtlasTexture(gfx::Context& context, bool deferred_data_init)
{
    META_FUNCTION_TASK();
    Ptr<gfx::Texture> atlas_texture_ptr = gfx::Texture::CreateImage(context, gfx::Dimensions(m_atlas_pack_ptr->GetSize()), 1, gfx::PixelFormat::R8Unorm, false);
    atlas_texture_ptr->SetName(m_settings.description.name + " Font Atlas");
    if (deferred_data_init)
    {
        context.RequestDeferredAction(gfx::Context::DeferredAction::CompleteInitialization);
    }
    else
    {
        atlas_texture_ptr->SetData({
            gfx::Resource::SubResource(reinterpret_cast<Data::ConstRawPtr>(m_atlas_bitmap.data()), static_cast<Data::Size>(m_atlas_bitmap.size()))
        });
    }
    return { atlas_texture_ptr, deferred_data_init };
}

void Font::RemoveAtlasTexture(gfx::Context& context)
{
    META_FUNCTION_TASK();
    m_atlas_textures.erase(&context);
    context.Disconnect(*this);
}

bool Font::UpdateAtlasBitmap(bool deferred_textures_update)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL_DESCR(m_atlas_pack_ptr, "can not update atlas bitmap until atlas is packed");

    const gfx::FrameSize& atlas_size = m_atlas_pack_ptr->GetSize();
    if (m_atlas_bitmap.size() == atlas_size.GetPixelsCount())
        return false;

    // Clear old atlas content
    std::fill(m_atlas_bitmap.begin(), m_atlas_bitmap.end(), Data::Byte{});
    m_atlas_bitmap.resize(atlas_size.GetPixelsCount(), Data::Byte{});

    // Render glyphs to atlas bitmap
    for (const auto& [char_code, character] : m_char_by_code)
    {
        character.DrawToAtlas(m_atlas_bitmap, atlas_size.GetWidth());
    }

    UpdateAtlasTextures(deferred_textures_update);
    return true;
}

void Font::UpdateAtlasTextures(bool deferred_textures_update)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL_DESCR(m_atlas_pack_ptr, "can not update atlas textures until atlas is packed and bitmap is up to date");
    if (m_atlas_textures.empty())
        return;

    for(auto& [context_ptr, atlas_texture] : m_atlas_textures)
    {
        if (deferred_textures_update)
        {
            // Texture will be updated on GPU context completing initialization,
            // when next GPU Frame rendering is started and just before uploading data on GPU with upload command queue
            atlas_texture.is_update_required = true;
            context_ptr->RequestDeferredAction(gfx::Context::DeferredAction::CompleteInitialization);
        }
        else
        {
            META_CHECK_ARG_NOT_NULL(context_ptr);
            UpdateAtlasTexture(*context_ptr, atlas_texture);
        }
    }

    Emit(&IFontCallback::OnFontAtlasUpdated, *this);
}

void Font::UpdateAtlasTexture(gfx::Context& context, AtlasTexture& atlas_texture)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL_DESCR(atlas_texture.texture_ptr, "font atlas texture is not initialized");

    const gfx::FrameSize atlas_size = m_atlas_pack_ptr->GetSize();
    if (const gfx::Dimensions& texture_dimensions = atlas_texture.texture_ptr->GetSettings().dimensions;
        texture_dimensions.GetWidth() != atlas_size.GetWidth() || texture_dimensions.GetHeight() != atlas_size.GetHeight())
    {
        const Ptr<gfx::Texture> old_texture_ptr = atlas_texture.texture_ptr;
        atlas_texture.texture_ptr = CreateAtlasTexture(context, false).texture_ptr;
        Emit(&IFontCallback::OnFontAtlasTextureReset, *this, old_texture_ptr, atlas_texture.texture_ptr);
    }
    else
    {
        // TODO: Update only a region of atlas texture containing character bitmap
        atlas_texture.texture_ptr->SetData({
            gfx::Resource::SubResource(reinterpret_cast<Data::ConstRawPtr>(m_atlas_bitmap.data()), static_cast<Data::Size>(m_atlas_bitmap.size()))
        });
    }

    atlas_texture.is_update_required = false;
}

void Font::ClearAtlasTextures()
{
    META_FUNCTION_TASK();
    for(const auto& [context_ptr, atlas_texture] : m_atlas_textures)
    {
        if (!context_ptr)
            continue;

        context_ptr->Disconnect(*this);
        Emit(&IFontCallback::OnFontAtlasTextureReset, *this, atlas_texture.texture_ptr, nullptr);
    }
    m_atlas_textures.clear();
}

void Font::OnContextReleased(gfx::Context& context)
{
    META_FUNCTION_TASK();
    RemoveAtlasTexture(context);
}

void Font::OnContextCompletingInitialization(gfx::Context& context)
{
    META_FUNCTION_TASK();
    if (const auto atlas_texture_it = m_atlas_textures.find(&context);
        atlas_texture_it != m_atlas_textures.end() && atlas_texture_it->second.is_update_required)
    {
        UpdateAtlasTexture(context, atlas_texture_it->second);
    }
}

static constexpr Font::Char::Code g_line_break_code = static_cast<Font::Char::Code>('\n');

Font::Char::Type Font::Char::GetType(Font::Char::Code char_code)
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;

    Font::Char::Type type_mask = Font::Char::Type::Unknown;
    if (char_code > 255)
        return type_mask;

    if (char_code == g_line_break_code)
        type_mask |= Font::Char::Type::LineBreak;

    if (std::isspace(static_cast<int>(char_code)))
        type_mask |= Font::Char::Type::Whitespace;

    return type_mask;
}

Font::Char::Char(Code code)
    : m_code(code)
    , m_type_mask(GetType(code))
{
    META_FUNCTION_TASK();
}

Font::Char::Char(Code code, gfx::FrameRect rect, gfx::Point2I offset, gfx::Point2I advance, UniquePtr<Glyph>&& glyph_ptr)
    : m_code(code)
    , m_type_mask(GetType(code))
    , m_rect(std::move(rect))
    , m_offset(std::move(offset))
    , m_advance(std::move(advance))
    , m_visual_size(IsWhiteSpace() ? m_advance.GetX() : m_offset.GetX() + m_rect.size.GetWidth(),
                    IsWhiteSpace() ? m_advance.GetY() : m_offset.GetY() + m_rect.size.GetHeight())
    , m_glyph_ptr(std::move(glyph_ptr))
{
    META_FUNCTION_TASK();
}

void Font::Char::DrawToAtlas(Data::Bytes& atlas_bitmap, uint32_t atlas_row_stride) const
{
    META_FUNCTION_TASK();
    if (!m_rect.size)
        return; 

    // Verify glyph placement
    META_CHECK_ARG_GREATER_OR_EQUAL(m_rect.GetLeft(), 0);
    META_CHECK_ARG_GREATER_OR_EQUAL(m_rect.GetTop(),  0);
    META_CHECK_ARG_LESS(m_rect.GetRight(), atlas_row_stride + 1);
    META_CHECK_ARG_LESS(m_rect.GetBottom(), atlas_bitmap.size() / atlas_row_stride + 1);

    // Draw glyph to bitmap
    FT_Glyph ft_glyph = m_glyph_ptr->GetFTGlyph();
    ThrowFreeTypeError(FT_Glyph_To_Bitmap(&ft_glyph, FT_RENDER_MODE_NORMAL, nullptr, false));

    FT_Bitmap& ft_bitmap = reinterpret_cast<FT_BitmapGlyph>(ft_glyph)->bitmap;
    META_CHECK_ARG_EQUAL(ft_bitmap.width, m_rect.size.GetWidth());
    META_CHECK_ARG_EQUAL(ft_bitmap.rows, m_rect.size.GetHeight());

    // Copy glyph pixels to output bitmap row-by-row
    for (uint32_t y = 0; y < ft_bitmap.rows; y++)
    {
        const uint32_t atlas_index = m_rect.origin.GetX() + (m_rect.origin.GetY() + y) * atlas_row_stride;
        META_CHECK_ARG_LESS_DESCR(atlas_index, atlas_bitmap.size() - ft_bitmap.width + 1, "char glyph does not fit into target atlas bitmap");
        std::copy(reinterpret_cast<Data::RawPtr>(ft_bitmap.buffer + y * ft_bitmap.width),
                  reinterpret_cast<Data::RawPtr>(ft_bitmap.buffer + (y + 1) * ft_bitmap.width),
                  atlas_bitmap.begin() + atlas_index);
    }
}

uint32_t Font::Char::GetGlyphIndex() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL_DESCR(m_glyph_ptr, "no glyph is available for character with code {}", static_cast<uint32_t>(m_code));
    return m_glyph_ptr->GetFaceIndex();
}

} // namespace Methane::Graphics
