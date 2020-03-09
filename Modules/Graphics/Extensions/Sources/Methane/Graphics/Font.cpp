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

FILE: Methane/Graphics/Text.cpp
Screen Quad rendering primitive.

******************************************************************************/

#include <Methane/Graphics/Font.h>

#include <Methane/Instrumentation.h>

#include <cassert>

extern "C"
{
#include <ft2build.h>
#include <freetype/ftglyph.h>
#include FT_FREETYPE_H
}

namespace Methane::Graphics
{

static const char* GetFreeTypeErrorMessage(FT_Error err)
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
        throw std::runtime_error(error_msg + GetFreeTypeErrorMessage(error));
    }
}

class Font::Library::Impl
{
public:
    Impl()
    {
        ITT_FUNCTION_TASK();
        ThrowFreeTypeError(FT_Init_FreeType(&m_p_library));
    }

    ~Impl()
    {
        ITT_FUNCTION_TASK();
        FT_Done_FreeType(m_p_library);
    }

    FT_Library GetFreeTypeLib() const { return m_p_library; }

private:
    FT_Library m_p_library;
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
{
    ITT_FUNCTION_TASK();
    Data::Chunk font_data = data_provider.GetData(m_settings.font_path);

    FT_Face     ft_face = nullptr;
    FT_Library  ft_library = Library::Get().GetImpl().GetFreeTypeLib();
    ThrowFreeTypeError(FT_New_Memory_Face(ft_library, static_cast<const FT_Byte*>(font_data.p_data), static_cast<FT_Long>(font_data.size), 0, &ft_face));
    FT_Set_Char_Size(ft_face,
                     m_settings.font_size_pt * 64, 0, // font Size is measured in 1/64ths of pixels; horizontal and vertical sizes are equal
                     m_settings.resolution_dpi, 0);   // font resolutions are equal for horizontal and vertical

    for (const char letter : m_settings.letters)
    {
        // TODO: check that letters do not repeat
        uint32_t char_index = FT_Get_Char_Index(ft_face, static_cast<FT_ULong>(letter));
        if (!char_index)
        {
            const std::string error_message = "Character " + std::to_string(letter) + " does not exist in font " + m_settings.font_path;
            throw std::runtime_error(error_message);
        }

        ThrowFreeTypeError(FT_Load_Glyph(ft_face, char_index, FT_LOAD_RENDER));

        FT_Glyph ft_glyph_atlas = nullptr;
        ThrowFreeTypeError(FT_Get_Glyph(ft_face->glyph, &ft_glyph_atlas));

        // All metrics dimensions are multiplied by 64, so we have to divide by 64
        int x_offset  = static_cast<int>(ft_face->glyph->metrics.horiBearingX >> 6);
        int y_offset  = static_cast<int>(ft_face->glyph->metrics.horiBearingY >> 6);
        int width     = static_cast<int>(ft_face->glyph->metrics.width >> 6);
        int height    = static_cast<int>(ft_face->glyph->metrics.height >> 6);
        int x_advance = static_cast<int>(ft_face->glyph->metrics.horiAdvance >> 6);
    }
}

} // namespace Methane::Graphics
