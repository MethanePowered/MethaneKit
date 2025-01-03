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

FILE: Methane/UserInterface/FontChar.h
Font char internal implementation.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Rect.hpp>
#include <Methane/Data/EnumMask.hpp>
#include <Methane/Data/RectBinPack.hpp>
#include <Methane/Data/Types.h>
#include <Methane/Memory.hpp>

#ifndef FT_Glyph
typedef struct FT_GlyphRec_*  FT_Glyph; // NOSONAR - typedef instead of using
#endif

namespace Methane::UserInterface
{

namespace gfx = Methane::Graphics;

class FontChar
{
public:
    class Glyph;

    enum class Type : uint8_t
    {
        Whitespace,
        LineBreak
    };

    using Code = char32_t;
    using TypeMask = Data::EnumMask<Type>;
    static TypeMask GetTypeMask(Code code);

    class Glyph // NOSONAR - custom destructor is required
    {
    public:
        Glyph(FT_Glyph ft_glyph, uint32_t face_index);
        ~Glyph();

        Glyph(const Glyph&) noexcept = delete;
        Glyph(Glyph&&) noexcept = default;

        Glyph& operator=(const Glyph&) noexcept = delete;
        Glyph& operator=(Glyph&&) noexcept = default;

        [[nodiscard]] FT_Glyph GetFreeTypeGlyph() const { return m_ft_glyph; }
        [[nodiscard]] uint32_t GetFaceIndex() const     { return m_face_index; }

    private:
        FT_Glyph m_ft_glyph;
        uint32_t m_face_index;
    };

    class BinPack
        : public Data::RectBinPack<gfx::FrameRect>
    {
    public:
        using FrameBinPack = Data::RectBinPack<gfx::FrameRect>;
        using FrameBinPack::RectBinPack;

        bool TryPack(const Refs<FontChar>& font_chars);
        bool TryPack(FontChar& font_char);
    };

    FontChar() = default;
    explicit FontChar(Code code);
    FontChar(Code code, gfx::FrameRect rect, gfx::Point2I offset, gfx::Point2I advance,
             FT_Glyph ft_glyph, uint32_t face_index);

    [[nodiscard]] Code GetCode() const noexcept
    { return m_code; }

    [[nodiscard]] bool IsLineBreak() const noexcept
    { return m_type_mask.HasAnyBit(Type::LineBreak); }

    [[nodiscard]] bool IsWhiteSpace() const noexcept
    { return m_type_mask.HasAnyBit(Type::Whitespace); }

    [[nodiscard]] const gfx::FrameRect& GetRect() const noexcept
    { return m_rect; }

    [[nodiscard]] const gfx::Point2I& GetOffset() const noexcept
    { return m_offset; }

    [[nodiscard]] const gfx::Point2I& GetAdvance() const noexcept
    { return m_advance; }

    [[nodiscard]] const gfx::FrameSize& GetVisualSize() const noexcept
    { return m_visual_size; }

    [[nodiscard]] friend bool operator<(const FontChar& left, const FontChar& right) noexcept
    { return left.m_rect.size.GetPixelsCount() < right.m_rect.size.GetPixelsCount(); }

    [[nodiscard]] friend bool operator>(const FontChar& left, const FontChar& right) noexcept
    { return left.m_rect.size.GetPixelsCount() > right.m_rect.size.GetPixelsCount(); }

    [[nodiscard]] explicit operator bool() const noexcept
    { return m_code != 0U; }

    void DrawToAtlas(Data::Bytes& atlas_bitmap, uint32_t atlas_row_stride) const;
    uint32_t GetGlyphIndex() const;

private:
    const Code     m_code = 0U;
    const TypeMask m_type_mask{};
    gfx::FrameRect m_rect;
    gfx::Point2I   m_offset;
    gfx::Point2I   m_advance;
    gfx::FrameSize m_visual_size;
    Ptr<Glyph>     m_glyph_ptr;
};

using FontChars = Refs<const FontChar>;

} // namespace Methane::UserInterface
