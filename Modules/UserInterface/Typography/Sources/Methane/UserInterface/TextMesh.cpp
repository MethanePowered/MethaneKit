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

FILE: Methane/UserInterface/TextMesh.cpp
Methane text mesh generation helper.

******************************************************************************/

#include "TextMesh.h"

#include <Methane/Graphics/RenderCommandList.h>
#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Instrumentation.h>

#include <stdexcept>

namespace Methane::UserInterface
{

enum class CharAction
{
    Continue,
    Wrap,
    Stop,
};

using ProcessFontCharAtPosition = const std::function<CharAction(const Font::Char& text_char, const gfx::FramePoint& char_pos, size_t char_index)>;

static void ForEachTextCharacterInRange(Font& font, const Font::Chars& text_chars, size_t begin_index, size_t end_index,
                                        TextMesh::CharPositions& char_positions, uint32_t viewport_width, Text::Wrap wrap,
                                        const ProcessFontCharAtPosition& process_char_at_position)
{
    META_FUNCTION_TASK();
    assert(!char_positions.empty());
    TextMesh::CharPosition char_pos = char_positions.back();
    const Font::Char* p_prev_text_char = nullptr;

    for (size_t char_index = begin_index; char_index < end_index; ++char_index)
    {
        const Font::Char& text_char = text_chars[char_index].get();
        assert(!!text_char);

        // Wrap to next line break on "line break" character or when text overruns viewport width
        if (text_char.IsLineBreak() || (wrap == Text::Wrap::Anywhere && viewport_width && char_pos.GetX() + text_char.GetRect().size.width > viewport_width))
        {
            char_pos = { 0u, char_pos.GetY() + font.GetLineHeight() };
            p_prev_text_char = nullptr;
        }

        // Skip visualization of "line break" character
        if (text_char.IsLineBreak())
        {
            char_positions.emplace_back(char_pos);
            continue;
        }

        if (!p_prev_text_char && char_index && !text_chars[char_index - 1].get().IsLineBreak())
            p_prev_text_char = &(text_chars[char_index - 1].get());

        if (p_prev_text_char)
            char_pos += font.GetKerning(*p_prev_text_char, text_char);

        CharAction action = process_char_at_position(text_char, char_pos, char_index);
        switch (action)
        {
        case CharAction::Continue:
            char_pos.SetX(char_pos.GetX() + text_char.GetAdvance().GetX());
            char_positions.emplace_back(char_pos);
            p_prev_text_char = &text_char;
            break;

        case CharAction::Wrap:
            char_pos = { 0u, char_pos.GetY() + font.GetLineHeight() };
            char_positions.emplace_back(char_pos);
            p_prev_text_char = nullptr;
            break;

        case CharAction::Stop:
            return;
        }
    }
}

static void ForEachTextCharacter(const std::u32string& text, Font& font, TextMesh::CharPositions& char_positions,
                                 uint32_t viewport_width, Text::Wrap wrap, const ProcessFontCharAtPosition& process_char_at_position)
{
    META_FUNCTION_TASK();
    const Font::Chars text_chars = font.GetTextChars(text);
    const ProcessFontCharAtPosition& word_wrap_char_at_position = // word wrap mode processor
        [&](const Font::Char& text_char, const gfx::FramePoint& cur_char_pos, size_t char_index) -> CharAction
        {
            if (text_char.IsWhiteSpace())
            {
                // Word wrap prediction: check if next word fits in given viewport width
                bool word_wrap_required = false;
                const gfx::FramePoint start_char_pos = { cur_char_pos.GetX() + text_char.GetAdvance().GetX(), cur_char_pos.GetY() };
                const size_t start_chars_count = char_positions.size();
                char_positions.emplace_back(start_char_pos);
                ForEachTextCharacterInRange(font, text_chars, char_index + 1, text_chars.size(), char_positions, viewport_width, Text::Wrap::Anywhere,
                    [&word_wrap_required, &start_char_pos, &text_chars](const Font::Char& text_char, const gfx::FramePoint& char_pos, size_t char_index) -> CharAction
                    {
                        // Word has ended if whitespace character is received or line break character was passed
                        if (text_char.IsWhiteSpace() || (char_index && text_chars[char_index - 1].get().IsLineBreak()))
                            return CharAction::Stop;

                        word_wrap_required = char_pos.GetY() > start_char_pos.GetY();
                        return word_wrap_required ? CharAction::Stop : CharAction::Continue;
                    }
                );
                char_positions.erase(char_positions.begin() + start_chars_count, char_positions.end());
                if (word_wrap_required)
                    return CharAction::Wrap;
            }
            return process_char_at_position(text_char, cur_char_pos, char_index);
        };
    ForEachTextCharacterInRange(font, text_chars, 0, text_chars.size(), char_positions, viewport_width, wrap,
                                wrap == Text::Wrap::Word && viewport_width ? word_wrap_char_at_position : process_char_at_position);
}

TextMesh::TextMesh(const std::u32string& text, Text::Layout layout, Font& font, gfx::FrameSize& viewport_size)
    : m_font(font)
    , m_layout(std::move(layout))
    , m_viewport_size(viewport_size)
{
    META_FUNCTION_TASK();
    Update(text, viewport_size);
}

bool TextMesh::IsUpdatable(const std::u32string& text, const Text::Layout& layout, Font& font, const gfx::FrameSize& viewport_size) const noexcept
{
    META_FUNCTION_TASK();
    // Text mesh can be updated when all text visualization parameters are equal to the initial
    // and new text start with the previously used text (typing continued),
    // or previous text starts with the new one (deleting with backspace)
    return m_viewport_size == viewport_size &&
           m_layout == layout &&
           std::addressof(m_font) == std::addressof(font) &&
           (IsNewTextStartsWithOldOne(text) || IsOldTextStartsWithNewOne(text));
}

void TextMesh::Update(const std::u32string& text, gfx::FrameSize& viewport_size)
{
    META_FUNCTION_TASK();
    const bool new_text_starts_with_old_one = IsNewTextStartsWithOldOne(text);
    const bool old_text_starts_with_new_one = IsOldTextStartsWithNewOne(text);

    if (m_viewport_size != viewport_size || (!new_text_starts_with_old_one && !old_text_starts_with_new_one))
    {
        throw std::invalid_argument("Text mesh can not be updated with a given text and viewport size");
    }

    if (new_text_starts_with_old_one)
    {
        AppendChars(text.substr(m_text.length()));
    }
    else if (old_text_starts_with_new_one)
    {
        EraseTrailingChars(m_text.length() - text.length(), true, true);
    }

    if (viewport_size)
        return;

    // Update zero viewport sizes by calculated content size
    if (!viewport_size.width)
    {
        viewport_size.width = m_content_size.width;
    }
    if (!viewport_size.height)
    {
        viewport_size.height = m_content_size.height;
    }

    return;
}

void TextMesh::EraseTrailingChars(size_t erase_chars_count, bool fixup_whitespace, bool update_content_size)
{
    META_FUNCTION_TASK();
    if (!erase_chars_count)
        return;

    if (erase_chars_count > m_text.length())
        throw std::invalid_argument("Unable to erase " + std::to_string(erase_chars_count) +
                                    " characters, more than text length (" + std::to_string(m_text.length()) + ").");

    const size_t erase_chars_from_index = m_text.length() - erase_chars_count;
    const size_t empty_symbols_count    = std::count_if(m_text.begin() + erase_chars_from_index, m_text.end(),
                                            [](char32_t char_code) { return char_code < 255 && (char_code == '\n' || std::isspace(static_cast<int>(char_code))); });
    const size_t erase_symbols_count    = erase_chars_count - empty_symbols_count;

    m_char_positions.erase(m_char_positions.begin() + m_char_positions.size() - erase_chars_count, m_char_positions.end());
    m_vertices.erase(m_vertices.begin() + m_vertices.size() - erase_symbols_count * 4, m_vertices.end());
    m_indices.erase( m_indices.begin()  + m_indices.size()  - erase_symbols_count * 6, m_indices.end());
    m_text.erase(m_text.begin() + erase_chars_from_index, m_text.end());

    if (fixup_whitespace && m_last_whitespace_index >= m_text.length())
    {
        const auto whitespace_it = std::find_if(m_text.rbegin(), m_text.rend(), [](char32_t char_code)
            { return char_code <= 255 && std::isspace(static_cast<int>(char_code)); }
        );
        m_last_whitespace_index = whitespace_it == m_text.rend() ? std::string::npos : std::distance(m_text.begin(), whitespace_it.base());
    }

    if (update_content_size)
    {
        UpdateContentSize();
    }
}

void TextMesh::AppendChars(std::u32string added_text)
{
    META_FUNCTION_TASK();
    if (added_text.empty())
        return;

    // Start adding new text characters from the previous text word so that it can be properly wrapped
    if (m_layout.wrap == Text::Wrap::Word && m_last_whitespace_index != std::string::npos)
    {
        // Remove characters starting with last whitespace and other non-whitespace symbols
        assert(m_last_whitespace_index < m_text.length());
        added_text = m_text.substr(m_last_whitespace_index) + added_text;
        EraseTrailingChars(m_text.length() - m_last_whitespace_index, false, false);
        m_last_whitespace_index = std::string::npos;
    }

    const size_t init_text_length  = m_text.length();
    const size_t added_text_length = added_text.length();

    m_text.insert(m_text.end(), added_text.begin(), added_text.end());

    const gfx::FrameSize& atlas_size = m_font.GetAtlasSize();
    m_vertices.reserve(m_vertices.size() + added_text_length * 4);
    m_indices.reserve(m_indices.size() + added_text_length * 6);

    if (m_char_positions.empty())
    {
        m_char_positions.emplace_back(CharPosition{ 0, m_font.GetLineHeight() });
    }
    m_char_positions.reserve(m_char_positions.size() + added_text.length());

    ForEachTextCharacter(added_text, m_font, m_char_positions, m_viewport_size.width, m_layout.wrap,
        [&](const Font::Char& font_char, const gfx::FramePoint& char_pos, size_t char_index) -> CharAction
        {
            if (font_char.IsWhiteSpace())
            {
                m_last_whitespace_index = char_index;
                return CharAction::Continue;
            }

            if (font_char.IsLineBreak())
                return CharAction::Continue;

            AddCharQuad(font_char, char_pos, atlas_size);
            UpdateContentSizeWithChar(font_char, char_pos);
            return CharAction::Continue;
        }
    );

    if (m_last_whitespace_index != std::string::npos)
        m_last_whitespace_index += init_text_length;
}

void TextMesh::AddCharQuad(const Font::Char& font_char, const gfx::FramePoint& char_pos, const gfx::FrameSize& atlas_size)
{
    META_FUNCTION_TASK();

    // Char quad rectangle in text model coordinates [0, 0] x [width, height]
    const gfx::Rect<float, float> ver_rect {
        {
            static_cast<float>(char_pos.GetX() + font_char.GetOffset().GetX()),
            static_cast<float>(char_pos.GetY() + font_char.GetOffset().GetY() + static_cast<int32_t>(font_char.GetRect().size.height)) * -1.f,
        },
        {
            static_cast<float>(font_char.GetRect().size.width),
            static_cast<float>(font_char.GetRect().size.height),
        }
    };

    // Char atlas rectangle in texture coordinates [0, 1] x [0, 1]
    const gfx::Rect<float, float> tex_rect {
        {
            static_cast<float>(font_char.GetRect().origin.GetX()) / atlas_size.width,
            static_cast<float>(font_char.GetRect().origin.GetY()) / atlas_size.height,
        },
        {
            static_cast<float>(font_char.GetRect().size.width)  / atlas_size.width,
            static_cast<float>(font_char.GetRect().size.height) / atlas_size.height,
        }
    };

    if (m_vertices.size() + 6 > std::numeric_limits<Index>::max())
        throw std::runtime_error("Text mesh index buffer overflow.");

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
    m_content_size = { 0u, 0u };
    for(uint32_t vertex_index = 2; vertex_index < m_vertices.size(); vertex_index += 4)
    {
        m_content_size.width  = std::max(m_content_size.width,  static_cast<uint32_t>(m_vertices[vertex_index].position[0]));
        m_content_size.height = std::max(m_content_size.height, static_cast<uint32_t>(-m_vertices[vertex_index].position[1]));
    }
}

void TextMesh::UpdateContentSizeWithChar(const Font::Char& font_char, const gfx::FramePoint& char_pos)
{
    META_FUNCTION_TASK();
    m_content_size.width  = std::max(m_content_size.width,  char_pos.GetX() + font_char.GetOffset().GetX() + font_char.GetRect().size.width);
    m_content_size.height = std::max(m_content_size.height, char_pos.GetY() + font_char.GetOffset().GetY() + font_char.GetRect().size.height);
}

} // namespace Methane::Graphics
