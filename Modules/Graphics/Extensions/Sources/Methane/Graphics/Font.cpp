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

#include <Methane/Instrumentation.h>

#include <map>
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
    ITT_FUNCTION_TASK();

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
        ITT_FUNCTION_TASK();
        ThrowFreeTypeError(FT_Init_FreeType(&m_ft_library));
    }

    ~Impl()
    {
        ITT_FUNCTION_TASK();
        FT_Done_FreeType(m_ft_library);
    }

    FT_Library GetFTLib() const { return m_ft_library; }

private:
    FT_Library m_ft_library;
};

class Font::Char::Glyph
{
public:
    Glyph(FT_Glyph ft_glyph)
        : m_ft_glyph(ft_glyph)
    {
        ITT_FUNCTION_TASK();
    }

    ~Glyph()
    {
        ITT_FUNCTION_TASK();
        FT_Done_Glyph(m_ft_glyph);
    }

    FT_Glyph GetFTGlyph() const { return m_ft_glyph; }

private:
    FT_Glyph m_ft_glyph;
};

class Font::Face
{
public:
    Face(const Data::Chunk& font_data)
        : m_ft_face(LoadFace(Library::Get().GetImpl().GetFTLib(), font_data))
        , m_has_kerning(static_cast<bool>(FT_HAS_KERNING(m_ft_face)))
    {
        ITT_FUNCTION_TASK();
    }

    ~Face()
    {
        ITT_FUNCTION_TASK();
        FT_Done_Face(m_ft_face);
    }

    void SetSize(uint32_t font_size_pt, uint32_t resolution_dpi)
    {
        ITT_FUNCTION_TASK();
        // font Size is measured in 1/64ths of pixels
        // 0 values mean that vertical value is equal horizontal value
        ThrowFreeTypeError(FT_Set_Char_Size(m_ft_face, font_size_pt * 64, 0, resolution_dpi, 0));
    }

    uint32_t GetCharIndex(Char::Code char_code)
    {
        ITT_FUNCTION_TASK();
        return FT_Get_Char_Index(m_ft_face, static_cast<FT_ULong>(char_code));
    }

    Char LoadChar(Char::Code char_code)
    {
        ITT_FUNCTION_TASK();

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
        return Char {
            char_code,
            {
                Point2i(static_cast<int32_t>(m_ft_face->glyph->metrics.horiAdvance >> 6),
                        static_cast<int32_t>(m_ft_face->glyph->metrics.vertAdvance >> 6)),
                FrameSize(static_cast<uint32_t>(m_ft_face->glyph->metrics.width >> 6),
                          static_cast<uint32_t>(m_ft_face->glyph->metrics.height >> 6))
            },
            Point2i(static_cast<int32_t>(m_ft_face->glyph->metrics.horiBearingX >> 6),
                    static_cast<int32_t>(m_ft_face->glyph->metrics.horiBearingY >> 6)),
            Point2i(),
            std::make_unique<Char::Glyph>(ft_glyph)
        };
    }

    int32_t GetKerning(uint32_t left_glyph_index, uint32_t right_glyph_index)
    {
        ITT_FUNCTION_TASK();
        if (!left_glyph_index || !right_glyph_index)
            return 0;

        FT_Vector kerning_vec = {};
        ThrowFreeTypeError(FT_Get_Kerning(m_ft_face, left_glyph_index, right_glyph_index, FT_KERNING_DEFAULT, &kerning_vec));
        return kerning_vec.x >> 6;
    }

    FT_Face GetFTFace() const noexcept { return m_ft_face; }

private:
    static FT_Face LoadFace(FT_Library ft_library, const Data::Chunk& font_data)
    {
        ITT_FUNCTION_TASK();

        FT_Face ft_face = nullptr;
        ThrowFreeTypeError(FT_New_Memory_Face(ft_library,
            static_cast<const FT_Byte*>(font_data.p_data),
            static_cast<FT_Long>(font_data.size), 0,
            &ft_face));

        return ft_face;
    }

    const FT_Face m_ft_face = nullptr;
    const bool    m_has_kerning;
};

class CharPack
{
public:
    CharPack(FrameSize size)
        : m_root(FrameRect{ Point2i(), std::move(size) })
    {
        ITT_FUNCTION_TASK();
    }

    bool TryPack(Font::Chars& font_chars)
    {
        ITT_FUNCTION_TASK();
        // Sort chars by increasing of glyph pixels count and the pack into given frame size
        std::sort(font_chars.begin(), font_chars.end());
        for(Font::Char& font_char : font_chars)
        {
            if (!m_root.TryPack(font_char))
                return false;
        }
        return true;
    }

private:
    class Node
    {
    public:
        Node(FrameRect rect) : m_rect(std::move(rect)) { ITT_FUNCTION_TASK(); }

        bool IsEmpty() const noexcept { return !m_sp_leaf_1 && !m_sp_leaf_2; }

        bool TryPack(Font::Char& font_char)
        {
            ITT_FUNCTION_TASK();
            if (font_char.rect.size == FrameSize())
                return true;

            if (IsEmpty())
            {
                if (font_char.rect.size <= m_rect.size)
                {
                    const FrameSize delta = m_rect.size - font_char.rect.size;

                    // we split to give one very small leaf and one very big one
                    // because it allows more efficient use of space
                    // if you don't do this, the bottom right corner never gets used
                    if (delta.width < delta.height)
                    {
                        // split so the top is cut in half and the rest is one big rect below
                        m_sp_leaf_1 = std::make_unique<Node>(FrameRect{
                            Point2i(m_rect.origin.GetX() + font_char.rect.size.width + 2, m_rect.origin.GetY()),
                            FrameSize(m_rect.size.width - (font_char.rect.size.width + 2), font_char.rect.size.height + 2)
                        });
                        m_sp_leaf_2 = std::make_unique<Node>(FrameRect{
                            Point2i(m_rect.origin.GetX(), m_rect.origin.GetY() + font_char.rect.size.height + 2),
                            FrameSize(m_rect.size.width, font_char.rect.size.height - (font_char.rect.size.height + 2))
                        });
                    }
                    else
                    {
                        // m_pLeaf1 = left (cut in half)
                        m_sp_leaf_1 = std::make_unique<Node>(FrameRect{
                            Point2i(m_rect.origin.GetX(), m_rect.origin.GetY() + font_char.rect.size.height + 2),
                            FrameSize(m_rect.size.width + 2, m_rect.size.height - (font_char.rect.size.height + 2))
                        });
                        // m_pLeaf2 = right (not cut)
                        m_sp_leaf_2 = std::make_unique<Node>(FrameRect{
                            Point2i(m_rect.origin.GetX() + font_char.rect.size.width + 2, m_rect.origin.GetY()),
                            FrameSize(m_rect.size.width - (font_char.rect.size.width + 2), font_char.rect.size.height)
                        });

                    }

                    font_char.rect.origin.SetX(m_rect.origin.GetX() + 1);
                    font_char.rect.origin.SetY(m_rect.origin.GetY() + 1);
                    return true;
                }
                return false;
            }

            if (m_sp_leaf_1->TryPack(font_char))
                return true;

            return m_sp_leaf_2->TryPack(font_char);
        }

    private:

        const FrameRect m_rect;
        UniquePtr<Node> m_sp_leaf_1;
        UniquePtr<Node> m_sp_leaf_2;
    };

    Node m_root;
};

Font::Library& Font::Library::Get()
{
    ITT_FUNCTION_TASK();
    static Library s_library;
    return s_library;
}

const Ptr<Font>& Font::Library::Add(const Data::Provider& data_provider, const Settings& font_settings)
{
    ITT_FUNCTION_TASK();
    auto emplace_result = m_font_by_name.emplace(font_settings.name, Ptr<Font>(new Font(data_provider, font_settings)));
    if (!emplace_result.second)
        throw std::invalid_argument("Font with name \"" + font_settings.name + "\" already exists in library.");

    return emplace_result.first->second;
}

bool Font::Library::Has(const std::string& font_name) const
{
    ITT_FUNCTION_TASK();
    return m_font_by_name.count(font_name);
}

Font& Font::Library::Get(const std::string& font_name) const
{
    ITT_FUNCTION_TASK();
    const auto font_by_name_it = m_font_by_name.find(font_name);
    if (font_by_name_it == m_font_by_name.end())
        throw std::invalid_argument("There is no font with name \"" + font_name + "\" in library.");

    assert(font_by_name_it->second);
    return *font_by_name_it->second;
}

void Font::Library::Remove(const std::string& font_name)
{
    ITT_FUNCTION_TASK();
    const auto font_by_name_it = m_font_by_name.find(font_name);
    if (font_by_name_it != m_font_by_name.end())
        m_font_by_name.erase(font_by_name_it);
}

void Font::Library::Clear()
{
    ITT_FUNCTION_TASK();
    m_font_by_name.clear();
}

Font::Library::Library()
    : m_sp_impl(std::make_unique<Impl>())
{
    ITT_FUNCTION_TASK();
}

Font::Font(const Data::Provider& data_provider, const Settings& settings)
    : m_settings(settings)
    , m_sp_face(std::make_unique<Face>(data_provider.GetData(m_settings.font_path)))
{
    ITT_FUNCTION_TASK();

    m_sp_face->SetSize(m_settings.font_size_pt, m_settings.resolution_dpi);
    AddChars(m_settings.characters);
}

void Font::AddChars(const std::wstring& characters)
{
    ITT_FUNCTION_TASK();
    for (wchar_t character : characters)
    {
        AddChar(static_cast<Char::Code>(character));
    }
}

void Font::AddChar(Char::Code char_code)
{
    ITT_FUNCTION_TASK();
    if (!HasChar(char_code))
    {
        m_char_by_code.emplace(char_code, m_sp_face->LoadChar(char_code));
    }
}

bool Font::HasChar(Char::Code char_code)
{
    ITT_FUNCTION_TASK();
    return m_char_by_code.count(char_code);
}

const Font::Char& Font::GetChar(Char::Code char_code) const
{
    ITT_FUNCTION_TASK();
    static const Char s_none_char = {};
    const auto char_by_code_it = m_char_by_code.find(char_code);
    return char_by_code_it == m_char_by_code.end() ? s_none_char : char_by_code_it->second;
}

const Ptr<Texture>& Font::GetAtlasTexturePtr(Context& context)
{
    ITT_FUNCTION_TASK();
    const auto atlas_texture_it = m_atlas_textures.find(&context);
    if (atlas_texture_it != m_atlas_textures.end())
    {
        assert(!!atlas_texture_it->second);
        return atlas_texture_it->second;
    }

    // Transform char-map to vector of chars
    Chars font_chars;
    std::transform(m_char_by_code.begin(), m_char_by_code.end(), std::back_inserter(font_chars),
        [](const auto& code_and_char) { return code_and_char.second; }
    );

    // Pick square atlas size fitting all character glyphs packed in it
    uint32_t atlas_dimension = 256u;
    bool atlas_dimension_sufficient = true;
    do
    {
        atlas_dimension *= 2;
        CharPack char_pack(FrameSize(atlas_dimension, atlas_dimension));
        atlas_dimension_sufficient = char_pack.TryPack(font_chars);
    }
    while(!atlas_dimension_sufficient);

    // Render glyphs to texture buffer
    Data::Bytes atlas_bitmap(atlas_dimension * atlas_dimension, 0);
    for(const Char& font_char : font_chars)
    {
        font_char.DrawToAtlas(atlas_bitmap, atlas_dimension);
    }

    // Create atlas texture and render glyphs to it
    Ptr<Texture> sp_atlas_texture = Texture::CreateImage(context, Dimensions(atlas_dimension, atlas_dimension), 1, PixelFormat::R8Uint, false);
    sp_atlas_texture->SetData({ Resource::SubResource(reinterpret_cast<Data::ConstRawPtr>(atlas_bitmap.data()), atlas_bitmap.size()) });
    return m_atlas_textures.emplace(&context, std::move(sp_atlas_texture)).first->second;
}

void Font::RemoveAtlasTexture(Context& context)
{
    ITT_FUNCTION_TASK();
    m_atlas_textures.erase(&context);
}

void Font::ClearAtlasTextures()
{
    ITT_FUNCTION_TASK();
    m_atlas_textures.clear();
}

void Font::Char::DrawToAtlas(Data::Bytes& atlas_bitmap, uint32_t atlas_row_stride) const
{
    ITT_FUNCTION_TASK();
    if (!rect.size)
        return;

    // Draw glyph to bitmap
    FT_Glyph ft_glyph = sp_glyph->GetFTGlyph();
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

} // namespace Methane::Graphics
