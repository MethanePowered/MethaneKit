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

FILE: Methane/UserInterface/FontChar.cpp
Font char internal implementation.

******************************************************************************/

#include "FontChar.h"

#include <Methane/UserInterface/Font.h>

#include <ft2build.h>
#include <freetype/ftglyph.h>
#include FT_FREETYPE_H

namespace Methane::UserInterface
{

inline void ThrowFreeTypeError(FT_Error error)
{
    if (error)
        throw FreeTypeError(error);
}

bool FontChar::BinPack::TryPack(const Refs<FontChar>& font_chars)
{
    META_FUNCTION_TASK();
    return std::ranges::all_of(font_chars,
                               [this](const Ref<FontChar>& font_char) { return TryPack(font_char.get()); });
}

bool FontChar::BinPack::TryPack(FontChar& font_char)
{
    return FrameBinPack::TryPack(font_char.m_rect);
}

FontChar::Glyph::Glyph(FT_Glyph ft_glyph, uint32_t face_index)
    : m_ft_glyph(ft_glyph)
    , m_face_index(face_index)
{
}

FontChar::Glyph::~Glyph()
{
    META_FUNCTION_TASK();
    FT_Done_Glyph(m_ft_glyph);
}

static constexpr FontChar::Code g_line_break_code = static_cast<FontChar::Code>('\n');

FontChar::TypeMask FontChar::GetTypeMask(FontChar::Code char_code)
{
    META_FUNCTION_TASK();
    FontChar::TypeMask type_mask;
    if (char_code > 255)
        return type_mask;

    if (char_code == g_line_break_code)
        type_mask.SetBitOn(FontChar::Type::LineBreak);

    if (std::isspace(static_cast<int>(char_code)))
        type_mask.SetBitOn(FontChar::Type::Whitespace);

    return type_mask;
}

FontChar::FontChar(Code code)
    : m_code(code)
    , m_type_mask(GetTypeMask(code))
{ }

FontChar::FontChar(Code code, gfx::FrameRect rect, gfx::Point2I offset, gfx::Point2I advance,
                   FT_Glyph ft_glyph, uint32_t face_index)
    : m_code(code)
    , m_type_mask(GetTypeMask(code))
    , m_rect(std::move(rect))
    , m_offset(std::move(offset))
    , m_advance(std::move(advance))
    , m_visual_size(IsWhiteSpace() ? m_advance.GetX() : m_offset.GetX() + m_rect.size.GetWidth(),
                    IsWhiteSpace() ? m_advance.GetY() : m_offset.GetY() + m_rect.size.GetHeight())
    , m_glyph_ptr(std::make_shared<Glyph>(ft_glyph, face_index))
{ }

void FontChar::DrawToAtlas(Data::Bytes& atlas_bitmap, uint32_t atlas_row_stride) const
{
    META_FUNCTION_TASK();
    if (!m_rect.size)
        return;

    // Verify glyph placement
    META_CHECK_NOT_NULL_DESCR(m_glyph_ptr, "Font character glyph is not initialized");
    META_CHECK_GREATER_OR_EQUAL(m_rect.GetLeft(), 0);
    META_CHECK_GREATER_OR_EQUAL(m_rect.GetTop(),  0);
    META_CHECK_LESS_OR_EQUAL(m_rect.GetRight(), atlas_row_stride);
    META_CHECK_LESS_OR_EQUAL(m_rect.GetBottom(), atlas_bitmap.size() / atlas_row_stride);

    // Draw glyph to bitmap
    FT_Glyph ft_glyph = m_glyph_ptr->GetFreeTypeGlyph();
    ThrowFreeTypeError(FT_Glyph_To_Bitmap(&ft_glyph, FT_RENDER_MODE_NORMAL, nullptr, false));

    FT_Bitmap& ft_bitmap = reinterpret_cast<FT_BitmapGlyph>(ft_glyph)->bitmap; // NOSONAR
    META_CHECK_EQUAL(ft_bitmap.width, m_rect.size.GetWidth());
    META_CHECK_EQUAL(ft_bitmap.rows, m_rect.size.GetHeight());

    // Copy glyph pixels to output bitmap row-by-row
    for (uint32_t y = 0; y < ft_bitmap.rows; y++)
    {
        const uint32_t atlas_index = m_rect.origin.GetX() + (m_rect.origin.GetY() + y) * atlas_row_stride;
        META_CHECK_LESS_DESCR(atlas_index, atlas_bitmap.size() - ft_bitmap.width + 1, "char glyph does not fit into target atlas bitmap");
        std::copy(reinterpret_cast<Data::RawPtr>(ft_bitmap.buffer + y * ft_bitmap.width), // NOSONAR
                  reinterpret_cast<Data::RawPtr>(ft_bitmap.buffer + (y + 1) * ft_bitmap.width), // NOSONAR
                  atlas_bitmap.begin() + atlas_index);
    }
}

uint32_t FontChar::GetGlyphIndex() const
{
    META_FUNCTION_TASK();
    META_CHECK_NOT_NULL_DESCR(m_glyph_ptr, "no glyph is available for character with code {}", static_cast<uint32_t>(m_code));
    return m_glyph_ptr->GetFaceIndex();
}

} // namespace Methane::UserInterface
