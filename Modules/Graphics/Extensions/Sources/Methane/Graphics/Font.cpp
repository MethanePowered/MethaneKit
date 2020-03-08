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

extern "C"
{
#include <ft2build.h>
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

class Font::Library::Impl
{
public:
    Impl()
    {
        ITT_FUNCTION_TASK();
        FT_Error error = FT_Init_FreeType(&m_p_library);
        if (error)
        {
            std::string message = "Failed to initialize free type library, error occured: ";
            message += GetFreeTypeErrorMessage(error);
            throw std::runtime_error(message);
        }
    }

    ~Impl()
    {
        ITT_FUNCTION_TASK();
        FT_Done_FreeType(m_p_library);
    }

private:
    FT_Library m_p_library;
};

Font::Library& Font::Library::Get()
{
    ITT_FUNCTION_TASK();
    static Library s_library;
    return s_library;
}

Ptr<Font> Font::Library::AddFont(Settings font_settings)
{
    ITT_FUNCTION_TASK();
    Ptr<Font> sp_font(new Font(font_settings));
    m_font_by_name.emplace(font_settings.name, sp_font);
    return sp_font;
}

WeakPtr<Font> Font::Library::GetFont(std::string font_name)
{
    ITT_FUNCTION_TASK();
    return m_font_by_name[font_name];
}

Font::Library::Library()
    : m_sp_impl(std::make_unique<Impl>())
{
    ITT_FUNCTION_TASK();
}

Font::Font(Settings settings)
    : m_settings(std::move(settings))
{
    ITT_FUNCTION_TASK();
}

} // namespace Methane::Graphics
