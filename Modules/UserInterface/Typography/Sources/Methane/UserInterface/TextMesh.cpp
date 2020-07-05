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

using ProcessFontCharAtPosition = const std::function<CharAction(const Font::Char& text_char, const gfx::FrameRect::Point& char_pos, size_t char_index)>;

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
        [&](const Font::Char& text_char, const gfx::FrameRect::Point& cur_char_pos, size_t char_index) -> CharAction
        {
            if (text_char.IsWhiteSpace())
            {
                // Word wrap prediction: check if next word fits in given viewport width
                bool word_wrap_required = false;
                const gfx::FrameRect::Point start_char_pos = { cur_char_pos.GetX() + text_char.GetAdvance().GetX(), cur_char_pos.GetY() };
                const size_t start_chars_count = char_positions.size();
                char_positions.emplace_back(start_char_pos);
                ForEachTextCharacterInRange(font, text_chars, char_index + 1, text_chars.size(), char_positions, viewport_width, Text::Wrap::Anywhere,
                    [&word_wrap_required, &start_char_pos, &text_chars](const Font::Char& text_char, const gfx::FrameRect::Point& char_pos, size_t char_index) -> CharAction
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

TextMesh::TextMesh(const std::u32string& text, Text::Wrap wrap, Font& font, gfx::FrameSize& viewport_size)
    : m_font(font)
    , m_wrap(wrap)
    , m_viewport_size(viewport_size)
{
    META_FUNCTION_TASK();
    Update(text, viewport_size);
}

bool TextMesh::IsUpdatable(const std::u32string& text, Text::Wrap wrap, Font& font, const gfx::FrameSize& viewport_size) const noexcept
{
    META_FUNCTION_TASK();
    // Text mesh can be updated when all text visualization parameters are equal to the initial
    // and new text start with the previously used text, i.e. the text typing is continued
    return m_viewport_size == viewport_size &&
           m_wrap == wrap &&
           std::addressof(m_font) == std::addressof(font) &&
           text.length() > m_text.length() &&
           (m_text.empty() || text.find(m_text) == 0);
}

void TextMesh::Update(const std::u32string& text, gfx::FrameSize& viewport_size)
{
    META_FUNCTION_TASK();
    if (m_viewport_size != viewport_size ||
        text.length() <= m_text.length() ||
        (!m_text.empty() && text.find(m_text) != 0))
    {
        throw std::invalid_argument("Text mesh can not be updated with a given text and viewport size");
    }

    if (m_wrap == Text::Wrap::Word && m_last_whitespace_index != std::string::npos)
    {
        // Start adding new text characters from the previous text word so that it can be properly wrapped
        const size_t prev_text_length = m_text.length();
        assert(m_last_whitespace_index < prev_text_length);
        const size_t removed_trailing_text_length = prev_text_length - m_last_whitespace_index;
        
        assert(m_char_positions.size() == prev_text_length + 1);
        m_text.erase(m_text.begin() + m_last_whitespace_index, m_text.end());
        m_char_positions.erase(m_char_positions.begin() + m_last_whitespace_index + 1, m_char_positions.end());

        // removed chars start with one whitespace and other non-whitespace chars
        const size_t mesh_chars_count = m_vertices.size() / 4;
        assert(mesh_chars_count * 6 == m_indices.size());
        assert(mesh_chars_count >= removed_trailing_text_length - 1);
        const size_t new_non_whitespace_chars_count = mesh_chars_count - (removed_trailing_text_length - 1);
        m_vertices.erase(m_vertices.begin() + new_non_whitespace_chars_count * 4, m_vertices.end());
        m_indices.erase( m_indices.begin()  + new_non_whitespace_chars_count * 6, m_indices.end());

        m_last_whitespace_index      = std::string::npos;
    }

    const size_t init_text_length = m_text.length();
    const std::u32string added_text = text.substr(init_text_length);
    const size_t  added_text_length = added_text.length();

    m_text = text;

    if (!added_text_length)
        return;

    const gfx::FrameSize& atlas_size = m_font.GetAtlasSize();
    m_vertices.reserve(m_vertices.size() + added_text_length * 4);
    m_indices.reserve( m_indices.size()  + added_text_length * 6);

    if (m_char_positions.empty())
    {
        m_char_positions.emplace_back(CharPosition{ 0, m_font.GetLineHeight() });
    }
    m_char_positions.reserve(m_char_positions.size() + added_text.length());

    ForEachTextCharacter(added_text, m_font, m_char_positions, viewport_size.width, m_wrap,
        [&](const Font::Char& font_char, const gfx::FrameRect::Point& char_pos, size_t char_index) -> CharAction
        {
            if (font_char.IsWhiteSpace())
                m_last_whitespace_index = char_index;

            if (font_char.IsLineBreak())
                return CharAction::Continue;

            AddCharQuad(font_char, char_pos, atlas_size);
            UpdateContentSizeWithChar(font_char, char_pos);
            return CharAction::Continue;
        }
    );
    
    if (m_last_whitespace_index != std::string::npos)
        m_last_whitespace_index += init_text_length;

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

void TextMesh::UpdateContentSizeWithChar(const Font::Char& font_char, const gfx::FrameRect::Point& char_pos)
{
    META_FUNCTION_TASK();
    m_content_size.width  = std::max(m_content_size.width,  char_pos.GetX() + font_char.GetOffset().GetX() + font_char.GetRect().size.width);
    m_content_size.height = std::max(m_content_size.height, char_pos.GetY() + font_char.GetOffset().GetY() + font_char.GetRect().size.height);
}

void TextMesh::AddCharQuad(const Font::Char& font_char, const gfx::FrameRect::Point& char_pos, const gfx::FrameSize& atlas_size)
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

} // namespace Methane::Graphics
