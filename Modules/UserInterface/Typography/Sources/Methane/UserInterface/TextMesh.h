/******************************************************************************

Copyright 2020-2021 Evgeny Gorodetskiy

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

FILE: Methane/UserInterface/TextMesh.h
Methane text mesh generation helper.

******************************************************************************/

#pragma once

#include <Methane/UserInterface/Text.h>
#include <Methane/Graphics/Types.h>

#include <vector>

namespace Methane::UserInterface
{

namespace gfx = Methane::Graphics;
namespace rhi = Methane::Graphics::Rhi;

class TextMesh
{
public:
    struct Vertex
    {
        Data::RawVector2F position;
        Data::RawVector2F texcoord;
    };

    using Index         = uint16_t;
    using Indices       = std::vector<Index>;
    using Vertices      = std::vector<Vertex>;

    struct CharPosition : gfx::FramePoint
    {
        CharPosition(CoordinateType x, CoordinateType y, bool is_line_start = false);

        bool     is_line_start              = false; // start of new line: either after line break `\n` or text wrap
        bool     is_whitespace              = false;
        bool     is_line_break              = false;
        size_t   start_vertex_index         = std::numeric_limits<size_t>::max();
        uint32_t visual_width               = 0U;

        bool IsWhiteSpaceOrLineBreak() const noexcept { return is_whitespace || is_line_break; }
    };

    using CharPositions = std::vector<CharPosition>;

    TextMesh(const std::u32string& text, Text::Layout layout, Font& font, gfx::FrameSize& viewport_size);

    [[nodiscard]] bool IsUpdatable(const std::u32string& text, const Text::Layout& layout, Font& font, const gfx::FrameSize& viewport_size) const noexcept;
    void Update(const std::u32string& text, gfx::FrameSize& viewport_size);

    [[nodiscard]] const std::u32string& GetText() const noexcept              { return m_text; }
    [[nodiscard]] Font&                 GetFont() noexcept                    { return m_font; }
    [[nodiscard]] Text::Layout          GetLayout() const noexcept            { return m_layout; }
    [[nodiscard]] const gfx::FrameSize& GetFrameSize() const noexcept         { return m_frame_size; }
    [[nodiscard]] const gfx::FrameSize& GetContentSize() const noexcept       { return m_content_size; }
    [[nodiscard]] uint32_t              GetContentTopOffset() const noexcept  { return m_content_top_offset == std::numeric_limits<uint32_t>::max() ? 0U : m_content_top_offset; }

    [[nodiscard]] const Vertices& GetVertices() const noexcept                { return m_vertices; }
    [[nodiscard]] const Indices&  GetIndices() const noexcept                 { return m_indices; }

    [[nodiscard]] Data::Size      GetVertexSize() const noexcept              { return static_cast<Data::Size>(sizeof(Vertex)); }
    [[nodiscard]] Data::Size      GetVerticesDataSize() const noexcept        { return static_cast<Data::Size>(m_vertices.size() * sizeof(Vertex)); }

    [[nodiscard]] Data::Size      GetIndexSize() const noexcept               { return static_cast<Data::Size>(sizeof(Index)); }
    [[nodiscard]] Data::Size      GetIndicesDataSize() const noexcept         { return static_cast<Data::Size>(m_indices.size() * sizeof(Index)); }

private:
    void EraseTrailingChars(size_t erase_chars_count, bool fixup_whitespace, bool update_alignment_and_content_size);
    void AppendChars(std::u32string added_text);
    void AddCharQuad(const Font::Char& font_char, const gfx::FramePoint& char_pos, const gfx::FrameSize& atlas_size);
    void ApplyAlignmentOffset(const size_t aligned_text_length, const size_t line_start_index);
    int32_t GetLineWidth(size_t line_start_index) const;
    int32_t GetHorizontalLineAlignmentOffset(size_t line_start_index) const;
    float GetJustifiedWhitespaceWidth(size_t line_start_index) const;
    void UpdateContentSize();
    void UpdateContentSizeWithChar(const Font::Char& font_char, const gfx::FramePoint& char_pos);

    [[nodiscard]] bool IsNewTextStartsWithOldOne(std::u32string_view text) const noexcept
    { return m_text.empty() || (m_text.length() < text.length()   && text.find(m_text) == 0); }

    [[nodiscard]] bool IsOldTextStartsWithNewOne(std::u32string_view text) const noexcept
    { return !text.empty()  &&  text.length()   < m_text.length() && m_text.find(text) == 0; }

    std::u32string       m_text;
    Font&                m_font;
    const Text::Layout   m_layout;
    const gfx::FrameSize m_frame_size;
    gfx::FrameSize       m_content_size;
    uint32_t             m_content_top_offset = std::numeric_limits<uint32_t>::max(); // minimum distance from frame top border to character quads in first text line
    CharPositions        m_char_positions; // char positions without any hor/ver alignment
    size_t               m_last_whitespace_index = std::string::npos;
    size_t               m_last_line_start_index = 0U;
    Vertices             m_vertices;
    Indices              m_indices;
};

} // namespace Methane::Graphics
