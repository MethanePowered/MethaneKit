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

FILE: Methane/UserInterface/Font.cpp
Font atlas textures generation and fonts library management classes.

******************************************************************************/

#include "FontImpl.hpp"

#include <Methane/Pimpl.hpp>
#include <Methane/UserInterface/Font.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <codecvt>

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

FreeTypeError::FreeTypeError(FT_Error error)
    : std::runtime_error(fmt::format("Unexpected FreeType error occurred '{}'", GetFTErrorMessage(error)))
    , m_error(error)
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

META_PIMPL_METHODS_IMPLEMENT(Font);

Font::Font(const Library& font_lib, const Data::IProvider& data_provider, const Settings& settings)
    : m_impl_ptr(std::make_unique<Impl>(font_lib, *this, data_provider, settings))
{
}

const Font::Settings& Font::GetSettings() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetSettings();
}

void Font::Connect(Data::Receiver<IFontCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Connect(receiver);
}

void Font::Disconnect(Data::Receiver<IFontCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Disconnect(receiver);
}

void Font::ResetChars(const std::string& utf8_characters) const
{
    GetImpl(m_impl_ptr).ResetChars(utf8_characters);
}

void Font::ResetChars(const std::u32string& utf32_characters) const
{
    GetImpl(m_impl_ptr).ResetChars(utf32_characters);
}

void Font::AddChars(const std::string& utf8_characters) const
{
    GetImpl(m_impl_ptr).AddChars(utf8_characters);
}

void Font::AddChars(const std::u32string& utf32_characters) const
{
    GetImpl(m_impl_ptr).AddChars(utf32_characters);
}

void Font::AddChar(char32_t char_code) const
{
    GetImpl(m_impl_ptr).AddChar(char_code);
}

uint32_t Font::GetLineHeight() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetLineHeight();
}

const gfx::FrameSize& Font::GetMaxGlyphSize() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetMaxGlyphSize();
}

const gfx::FrameSize& Font::GetAtlasSize() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetAtlasSize();
}

const rhi::Texture& Font::GetAtlasTexture(const rhi::RenderContext& context) const
{
    return GetImpl(m_impl_ptr).GetAtlasTexture(context);
}

void Font::RemoveAtlasTexture(const rhi::RenderContext& context) const
{
    GetImpl(m_impl_ptr).RemoveAtlasTexture(context);
}

void Font::ClearAtlasTextures() const
{
    GetImpl(m_impl_ptr).ClearAtlasTextures();
}

Font::Impl& Font::GetImplementation()
{
    return *m_impl_ptr;
}

const Font::Impl& Font::GetImplementation() const
{
    return *m_impl_ptr;
}

} // namespace Methane::Graphics
