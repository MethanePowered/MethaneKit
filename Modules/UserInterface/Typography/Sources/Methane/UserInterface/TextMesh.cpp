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

FILE: Methane/UserInterface/TextMesh.cpp
Methane text mesh generation helper.

******************************************************************************/

#include "TextMesh.h"

#include <Methane/Graphics/RenderCommandList.h>
#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Data/TypeFormatters.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <stdexcept>

namespace Methane::UserInterface
{

enum class CharAction
{
    Continue,
    Wrap,
    Stop,
};

using IndexRange = std::pair<size_t, size_t>;

template<typename FuncType> // function CharAction(const Font::Char& text_char, const TextMesh::CharPosition& char_pos, size_t char_index)
void ForEachTextCharacterInRange(Font& font, const Font::Chars& text_chars, const IndexRange& index_range,
                                 TextMesh::CharPositions& char_positions, uint32_t frame_width, Text::Wrap wrap,
                                 FuncType process_char_at_position)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_EMPTY(char_positions);
    const Font::Char* p_prev_text_char = nullptr;

    for (size_t char_index = index_range.first; char_index < index_range.second; ++char_index)
    {
        const Font::Char& text_char = text_chars[char_index].get();
        META_CHECK_ARG_NOT_ZERO(text_char);

        TextMesh::CharPosition& char_pos = char_positions.back();
        char_pos.is_whitespace_or_linebreak = text_char.IsLineBreak() || text_char.IsWhiteSpace();
        char_pos.visual_width               = text_char.GetVisualSize().width;

        // Wrap to next line and skip visualization of "line break" character
        if (text_char.IsLineBreak())
        {
            char_positions.emplace_back(0, char_pos.GetY() + font.GetLineHeight(), true);
            p_prev_text_char = nullptr;
            continue;
        }

        // Wrap to next line on text overrun of frame width
        const uint32_t char_right_pos = char_pos.GetX() + (text_char.IsWhiteSpace() ? 0U : char_pos.visual_width);
        if (wrap == Text::Wrap::Anywhere && frame_width && char_right_pos > frame_width)
        {
            char_pos.SetX(0);
            char_pos.SetY(char_pos.GetY() + font.GetLineHeight());
            char_pos.is_line_start = true;
            p_prev_text_char = nullptr;
        }

        if (!p_prev_text_char && char_index && !text_chars[char_index - 1].get().IsLineBreak())
            p_prev_text_char = &(text_chars[char_index - 1].get());

        if (p_prev_text_char)
            char_pos += font.GetKerning(*p_prev_text_char, text_char);

        const CharAction action = process_char_at_position(text_char, char_pos, char_index);
        switch (action)
        {
        case CharAction::Continue:
            char_positions.emplace_back(char_pos.GetX() + text_char.GetAdvance().GetX(), char_pos.GetY());
            p_prev_text_char = &text_char;
            break;

        case CharAction::Wrap:
            char_positions.emplace_back(0, char_pos.GetY() + font.GetLineHeight(), true);
            p_prev_text_char = nullptr;
            break;

        case CharAction::Stop:
            return;

        default:
            META_UNEXPECTED_ENUM_ARG(action);
        }
    }
}

template<typename FuncType> // function CharAction(const Font::Char& text_char, const TextMesh::CharPosition& char_pos, size_t char_index)
static void ForEachTextCharacter(const std::u32string& text, Font& font, TextMesh::CharPositions& char_positions,
                                 uint32_t frame_width, Text::Wrap wrap, FuncType process_char_at_position)
{
    META_FUNCTION_TASK();
    const Font::Chars text_chars = font.GetTextChars(text);
    const IndexRange  text_range { 0, text_chars.size() };
    if (wrap == Text::Wrap::Word && frame_width)
    {
        ForEachTextCharacterInRange(font, text_chars, text_range, char_positions, frame_width, wrap,
            [&font, &text_chars, &char_positions, &frame_width, &process_char_at_position]
            (const Font::Char& text_char, const TextMesh::CharPosition& cur_char_pos, size_t char_index) -> CharAction
            {
                if (text_char.IsWhiteSpace())
                {
                    // Word wrap prediction: check if next word fits in given frame width
                    bool word_wrap_required = false;
                    const size_t start_chars_count = char_positions.size();
                    char_positions.emplace_back(cur_char_pos.GetX() + text_char.GetAdvance().GetX(), cur_char_pos.GetY());
                    ForEachTextCharacterInRange(font, text_chars, { char_index + 1, text_chars.size() }, char_positions, frame_width, Text::Wrap::Anywhere,
                        [&word_wrap_required, &cur_char_pos, &text_chars](const Font::Char& text_char, const gfx::FramePoint& char_pos, size_t char_index) -> CharAction
                        {
                            // Word has ended if whitespace character is received or line break character was passed
                            if (text_char.IsWhiteSpace() || (char_index && text_chars[char_index - 1].get().IsLineBreak()))
                                return CharAction::Stop;

                            word_wrap_required = char_pos.GetY() > cur_char_pos.GetY();
                            return word_wrap_required ? CharAction::Stop : CharAction::Continue;
                        }
                    );
                    char_positions.erase(char_positions.begin() + start_chars_count, char_positions.end());
                    if (word_wrap_required)
                        return CharAction::Wrap;
                }
                return process_char_at_position(text_char, cur_char_pos, char_index);
            }
        );
    }
    else
    {
        ForEachTextCharacterInRange(font, text_chars, text_range, char_positions, frame_width, wrap, process_char_at_position);
    }
}

TextMesh::CharPosition::CharPosition(CoordinateType x, CoordinateType y, bool is_line_start)
    : gfx::FramePoint(x, y)
    , is_line_start(is_line_start)
{
}

TextMesh::TextMesh(const std::u32string& text, Text::Layout layout, Font& font, gfx::FrameSize& frame_size)
    : m_font(font)
    , m_layout(layout)
    , m_frame_size(frame_size)
{
    META_FUNCTION_TASK();
    m_content_size.width = frame_size.width;

    Update(text, frame_size);
}

bool TextMesh::IsUpdatable(const std::u32string& text, const Text::Layout& layout, Font& font, const gfx::FrameSize& frame_size) const noexcept
{
    META_FUNCTION_TASK();
    // Text mesh can be updated when all text visualization parameters are equal to the initial
    // and new text start with the previously used text (typing continued),
    // or previous text starts with the new one (deleting with backspace)
    return m_frame_size == frame_size &&
           m_layout.wrap == layout.wrap &&
           m_layout.horizontal_alignment == layout.horizontal_alignment && // vertical_alignment is not handled in TextMesh
           std::addressof(m_font) == std::addressof(font) &&
           (IsNewTextStartsWithOldOne(text) || IsOldTextStartsWithNewOne(text));
}

void TextMesh::Update(const std::u32string& text, gfx::FrameSize& frame_size)
{
    META_FUNCTION_TASK();
    const bool new_text_starts_with_old_one = IsNewTextStartsWithOldOne(text);
    const bool old_text_starts_with_new_one = IsOldTextStartsWithNewOne(text);

    META_CHECK_ARG_EQUAL_DESCR(frame_size, m_frame_size, "text mesh can be incrementally updated only when frame size does not change");
    META_CHECK_ARG_NAME_DESCR("text", new_text_starts_with_old_one || old_text_starts_with_new_one, "text mesh can be incrementally updated only when text is appended or backspaced");

    if (new_text_starts_with_old_one)
    {
        AppendChars(text.substr(m_text.length()));
    }
    else if (old_text_starts_with_new_one)
    {
        EraseTrailingChars(m_text.length() - text.length(), true, true);
    }

    if (frame_size)
        return;

    // Update zero frame sizes by calculated content size
    if (!frame_size.width)
    {
        frame_size.width = m_content_size.width;
    }
    if (!frame_size.height)
    {
        frame_size.height = m_content_size.height - GetContentTopOffset();
    }

    return;
}

void TextMesh::EraseTrailingChars(size_t erase_chars_count, bool fixup_whitespace, bool update_alignment_and_content_size)
{
    META_FUNCTION_TASK();
    if (!erase_chars_count)
        return;

    META_CHECK_ARG_LESS_DESCR(erase_chars_count, m_text.length() + 1, "unable to erase mote characters than text contains");
    const size_t erase_chars_from_index = m_text.length() - erase_chars_count;
    const size_t empty_symbols_count    = std::count_if(m_text.begin() + erase_chars_from_index, m_text.end(),
                                            [](char32_t char_code) { return char_code < 255 && (char_code == '\n' || std::isspace(static_cast<int>(char_code))); });

    META_CHECK_ARG_LESS(empty_symbols_count, erase_chars_count + 1);
    const size_t erase_symbols_count = erase_chars_count - empty_symbols_count;

    META_CHECK_ARG_LESS(erase_chars_count, m_char_positions.size() + 1);
    META_CHECK_ARG_LESS(erase_symbols_count, m_vertices.size() / 4 + 1);
    META_CHECK_ARG_LESS(erase_symbols_count, m_indices.size()  / 6 + 1);

    m_char_positions.erase(m_char_positions.begin() + m_char_positions.size() - erase_chars_count, m_char_positions.end());
    m_vertices.erase(m_vertices.begin() + m_vertices.size() - erase_symbols_count * 4, m_vertices.end());
    m_indices.erase( m_indices.begin()  + m_indices.size()  - erase_symbols_count * 6, m_indices.end());
    m_text.erase(m_text.begin() + erase_chars_from_index, m_text.end());

    if (fixup_whitespace && m_last_whitespace_index >= m_text.length())
    {
        const auto whitespace_it = std::find_if(m_text.rbegin(), m_text.rend(),
            [](char32_t char_code) { return char_code <= 255 && std::isspace(static_cast<int>(char_code)); }
        );
        if (whitespace_it != m_text.rend())
        {
            m_last_whitespace_index = std::distance(m_text.begin(), whitespace_it.base()) - 1;
            META_CHECK_ARG(m_last_whitespace_index, m_char_positions[m_last_whitespace_index].is_whitespace_or_linebreak);
        }
        else
        {
            m_last_whitespace_index = std::string::npos;
        }
    }

    if (m_last_line_start_index >= m_text.length())
    {
        const auto line_start_it = std::find_if(m_char_positions.rbegin(), m_char_positions.rend(),
            [](const CharPosition& char_pos) { return char_pos.is_line_start; }
        );
        if (line_start_it != m_char_positions.rend())
        {
            m_last_line_start_index = std::distance(m_char_positions.begin(), line_start_it.base()) - 1;
            META_CHECK_ARG(m_last_line_start_index, m_char_positions[m_last_line_start_index].is_line_start);
        }
        else
        {
            m_last_line_start_index = std::string::npos;
        }
    }

    if (update_alignment_and_content_size)
    {
        UpdateContentSize();
        ApplyAlignmentOffset(m_text.length(), m_last_line_start_index);
    }
}

void TextMesh::AppendChars(std::u32string added_text)
{
    META_FUNCTION_TASK();
    if (added_text.empty())
        return;

    // Start adding new text characters from the previous text word (so that it can be properly wrapped) or from the last line start
    if (m_layout.wrap == Text::Wrap::Word && !m_text.empty() &&
         (m_last_whitespace_index != std::string::npos ||
          m_last_line_start_index != std::string::npos))
    {
        // Remove characters starting with last whitespace and other non-whitespace symbols
        const size_t update_from_index = m_last_whitespace_index != std::string::npos
                                       ? (m_last_line_start_index != std::string::npos ? std::max(m_last_whitespace_index, m_last_line_start_index) : m_last_whitespace_index)
                                       : m_last_line_start_index;
        if (update_from_index < m_text.length())
        {
            added_text = m_text.substr(update_from_index) + added_text;
            EraseTrailingChars(m_text.length() - update_from_index, false, false);
        }
        m_last_whitespace_index = std::string::npos;
    }

    const size_t init_text_length  = m_text.length();
    const size_t added_text_length = added_text.length();
    const size_t init_line_start_index = m_last_line_start_index;

    m_text.insert(m_text.end(), added_text.begin(), added_text.end());

    const gfx::FrameSize& atlas_size = m_font.GetAtlasSize();
    m_vertices.reserve(m_vertices.size() + added_text_length * 4);
    m_indices.reserve(m_indices.size() + added_text_length * 6);

    if (m_char_positions.empty())
    {
        m_char_positions.emplace_back(0, static_cast<int32_t>(m_font.GetLineHeight()), true);
    }
    m_char_positions.reserve(m_char_positions.size() + added_text.length());

    ForEachTextCharacter(added_text, m_font, m_char_positions, m_frame_size.width, m_layout.wrap,
                         [&](const Font::Char& font_char, const TextMesh::CharPosition& char_pos, size_t char_index) -> CharAction
        {
            if (font_char.IsWhiteSpace())
            {
                m_last_whitespace_index = init_text_length + char_index;
            }

            if (font_char.IsWhiteSpace() || font_char.IsLineBreak())
            {
                META_CHECK_ARG(char_index, m_char_positions[init_text_length + char_index].is_whitespace_or_linebreak);
                return CharAction::Continue;
            }

            if (char_pos.is_line_start)
            {
                m_last_line_start_index = init_text_length + char_index;
                META_CHECK_ARG(m_last_line_start_index, m_char_positions[m_last_line_start_index].is_line_start);
            }

            m_char_positions.back().start_vertex_index = m_vertices.size();

            AddCharQuad(font_char, char_pos, atlas_size);
            UpdateContentSizeWithChar(font_char, char_pos);
            return CharAction::Continue;
        }
    );

    if (m_char_positions.back().is_line_start)
        m_last_line_start_index = m_char_positions.size() - 1;

    ApplyAlignmentOffset(init_text_length, init_line_start_index);
}

void TextMesh::ApplyAlignmentOffset(const size_t aligned_text_length, const size_t line_start_index)
{
    META_FUNCTION_TASK();
    if (m_layout.horizontal_alignment == Text::HorizontalAlignment::Left)
        return;

    META_CHECK_ARG(line_start_index, m_char_positions[line_start_index].is_line_start);
    const size_t  end_char_index                  = m_char_positions.size() - 1;
    const int32_t frame_width                     = static_cast<int32_t>(m_content_size.width);
    int32_t       horizontal_alignment_offset     = 0; // Alignment offset of the recently appended character
    int32_t       horizontal_alignment_adjustment = 0; // Alignment adjustment of the existing character of the last line of text

    // Apply horizontal alignment offset to newly added and existing character quads at last line
    for(size_t char_index = line_start_index; char_index < end_char_index; ++char_index)
    {
        const CharPosition& char_position = m_char_positions[char_index];
        if (char_position.is_line_start && char_index <= end_char_index - 1)
        {
            horizontal_alignment_offset = GetHorizontalLineAlignmentOffset(char_index, frame_width);
            if (char_index < aligned_text_length)
            {
                // Calculate previously aligned characters adjustment
                META_CHECK_ARG_LESS(char_position.start_vertex_index, m_vertices.size());
                horizontal_alignment_adjustment = horizontal_alignment_offset - static_cast<int32_t>(m_vertices[char_position.start_vertex_index].position[0]);
            }
        }

        if (char_position.is_whitespace_or_linebreak)
            continue;

        // Apply line alignment offset to the character quad vertices
        META_CHECK_ARG_LESS(char_position.start_vertex_index, m_vertices.size());
        const int32_t alignment_offset = char_index < aligned_text_length ? horizontal_alignment_adjustment : horizontal_alignment_offset;
        for (size_t vertex_id = 0; vertex_id < 4; ++vertex_id)
        {
            m_vertices[char_position.start_vertex_index + vertex_id].position[0] += alignment_offset;
        }
    }
}

int32_t TextMesh::GetHorizontalLineAlignmentOffset(size_t line_start_index, int32_t frame_width) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG(line_start_index, m_char_positions[line_start_index].is_line_start);

    // Find next line start or end of text
    auto line_end_position_it = std::find_if(m_char_positions.begin() + line_start_index + 1, m_char_positions.end() - 1,
        [](const CharPosition& line_char_pos) { return line_char_pos.is_line_start; }
    );

    // Step back from next line start to get end of line position
    while(line_end_position_it->is_line_start && line_end_position_it != m_char_positions.begin())
        --line_end_position_it;

    // Calculate current line width and alignment offset
    const int32_t line_width = line_end_position_it->GetX() + line_end_position_it->visual_width - m_char_positions[line_start_index].GetX();
    switch(m_layout.horizontal_alignment)
    {
    case Text::HorizontalAlignment::Right:  return (frame_width - line_width);
    case Text::HorizontalAlignment::Center: return (frame_width - line_width) / 2;
    default:                                return 0;
    }
}

void TextMesh::AddCharQuad(const Font::Char& font_char, const gfx::FramePoint& char_pos, const gfx::FrameSize& atlas_size)
{
    META_FUNCTION_TASK();

    // Char quad rectangle in text model coordinates [0, 0] x [width, height]
    const gfx::Rect<float, float> ver_rect {
        {
            static_cast<float>(char_pos.GetX() + font_char.GetOffset().GetX()),
            static_cast<float>(char_pos.GetY() + font_char.GetOffset().GetY() + static_cast<int32_t>(font_char.GetRect().size.height)) * -1.F,
        },
        {
            static_cast<float>(font_char.GetRect().size.width),
            static_cast<float>(font_char.GetRect().size.height),
        }
    };

    // Char atlas rectangle in texture coordinates [0, 1] x [0, 1]
    const gfx::Rect<float, float> tex_rect {
        {
            static_cast<float>(font_char.GetRect().origin.GetX()) / static_cast<float>(atlas_size.width),
            static_cast<float>(font_char.GetRect().origin.GetY()) / static_cast<float>(atlas_size.height),
        },
        {
            static_cast<float>(font_char.GetRect().size.width)  / static_cast<float>(atlas_size.width),
            static_cast<float>(font_char.GetRect().size.height) / static_cast<float>(atlas_size.height),
        }
    };

    META_CHECK_ARG_LESS_DESCR(m_vertices.size(), std::numeric_limits<Index>::max() - 5, "text mesh index buffer overflow");
    const Index start_index = static_cast<Index>(m_vertices.size());

    m_vertices.emplace_back(Vertex{
        { ver_rect.GetLeft(), ver_rect.GetBottom() },
        { tex_rect.GetLeft(), tex_rect.GetTop() },
    });
    m_vertices.emplace_back(Vertex{
        { ver_rect.GetLeft(), ver_rect.GetTop() },
        { tex_rect.GetLeft(), tex_rect.GetBottom() },
    });
    m_vertices.emplace_back(Vertex{
        { ver_rect.GetRight(), ver_rect.GetTop() },
        { tex_rect.GetRight(), tex_rect.GetBottom() },
    });
    m_vertices.emplace_back(Vertex{
        { ver_rect.GetRight(), ver_rect.GetBottom() },
        { tex_rect.GetRight(), tex_rect.GetTop() },
    });

    m_indices.push_back(start_index);
    m_indices.push_back(start_index + 1);
    m_indices.push_back(start_index + 2);
    m_indices.push_back(start_index + 2);
    m_indices.push_back(start_index + 3);
    m_indices.push_back(start_index);
}

void TextMesh::UpdateContentSize()
{
    META_FUNCTION_TASK();
    m_content_size = { 0U, 0U };
    m_content_top_offset = std::numeric_limits<uint32_t>::max();
    for(uint32_t vertex_index = 0; vertex_index < m_vertices.size(); vertex_index += 4)
    {
        m_content_top_offset  = std::min(m_content_top_offset,  static_cast<uint32_t>(-m_vertices[vertex_index].position[1]));
        m_content_size.width  = std::max(m_content_size.width,  static_cast<uint32_t>( m_vertices[vertex_index + 2].position[0]));
        m_content_size.height = std::max(m_content_size.height, static_cast<uint32_t>(-m_vertices[vertex_index + 2].position[1]));
    }

    if (m_frame_size.width)
        m_content_size.width = m_frame_size.width;
}

void TextMesh::UpdateContentSizeWithChar(const Font::Char& font_char, const gfx::FramePoint& char_pos)
{
    META_FUNCTION_TASK();
    m_content_top_offset  = std::min(m_content_top_offset,  static_cast<uint32_t>(char_pos.GetY() + font_char.GetOffset().GetY()));
    m_content_size.width  = std::max(m_content_size.width,  char_pos.GetX() + font_char.GetVisualSize().width);
    m_content_size.height = std::max(m_content_size.height, char_pos.GetY() + font_char.GetVisualSize().height);
}

} // namespace Methane::Graphics
