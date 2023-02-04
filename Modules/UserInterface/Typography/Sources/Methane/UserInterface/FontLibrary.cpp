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

FILE: Methane/UserInterface/FontLibrary.h
Fonts library to manage created font instances.

******************************************************************************/

#include <Methane/UserInterface/FontLibrary.h>
#include <Methane/Data/Emitter.hpp>
#include <Methane/Pimpl.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace Methane::UserInterface
{

inline void ThrowFreeTypeError(FT_Error error)
{
    if (error)
        throw FreeTypeError(error);
}

class FontLibrary::Impl // NOSONAR - custom destructor is required
    : public Data::Emitter<IFontLibraryCallback>
{
public:
    explicit Impl(FontLibrary& font_lib)
        : m_font_lib(font_lib)
    {
        META_FUNCTION_TASK();
        ThrowFreeTypeError(FT_Init_FreeType(&m_ft_library));
    }

    ~Impl() override
    {
        META_FUNCTION_TASK();
        FT_Done_FreeType(m_ft_library);
    }

    Impl(const Impl&) noexcept = delete;
    Impl(Impl&&) noexcept = delete;

    Impl& operator=(const Impl&) noexcept = delete;
    Impl& operator=(Impl&&) noexcept = delete;

    [[nodiscard]] std::vector<Font> GetFonts() const
    {
        META_FUNCTION_TASK();
        std::vector<Font> fonts;
        for (const auto& [font_name, font]: m_font_by_name)
        {
            fonts.emplace_back(font);
        }
        return fonts;
    }

    [[nodiscard]] bool HasFont(const std::string& font_name) const
    {
        META_FUNCTION_TASK();
        return m_font_by_name.count(font_name);
    }

    [[nodiscard]] Font& GetFont(const std::string& font_name)
    {
        META_FUNCTION_TASK();
        const auto font_by_name_it = m_font_by_name.find(font_name);
        META_CHECK_ARG_DESCR(font_name, font_by_name_it != m_font_by_name.end(), "there is no font with a give name in fonts library");
        return font_by_name_it->second;
    }

    [[nodiscard]] Font& GetFont(const Data::IProvider& data_provider, const FontSettings& font_settings)
    {
        META_FUNCTION_TASK();
        const auto font_by_name_it = m_font_by_name.find(font_settings.description.name);
        if (font_by_name_it == m_font_by_name.end())
            return AddFont(data_provider, font_settings);

        return font_by_name_it->second;
    }

    Font& AddFont(const Data::IProvider& data_provider, const FontSettings& font_settings)
    {
        META_FUNCTION_TASK();
        auto [name_and_font_it, font_added] = m_font_by_name.try_emplace(font_settings.description.name, m_font_lib, data_provider, font_settings);
        META_CHECK_ARG_DESCR(font_settings.description.name, font_added, "font with a give name already exists in fonts library");

        Emit(&IFontLibraryCallback::OnFontAdded, name_and_font_it->second);

        return name_and_font_it->second;
    }

    void RemoveFont(const std::string& font_name)
    {
        META_FUNCTION_TASK();
        const auto font_by_name_it = m_font_by_name.find(font_name);
        if (font_by_name_it == m_font_by_name.end())
            return;

        Emit(&IFontLibraryCallback::OnFontRemoved, font_by_name_it->second);
        m_font_by_name.erase(font_by_name_it);
    }

    void Clear()
    {
        META_FUNCTION_TASK();
        m_font_by_name.clear();
    }

    [[nodiscard]] FT_Library GetFreeTypeLibrary() const
    {
        return m_ft_library;
    }

private:
    using FontByName = std::map<std::string, Font, std::less<std::string_view>>;

    FontLibrary& m_font_lib;
    FT_Library   m_ft_library;
    FontByName   m_font_by_name;
};

FontLibrary::FontLibrary()
    : m_impl_ptr(std::make_unique<Impl>(*this)) // NOSONAR
{
    META_FUNCTION_TASK();
}

void FontLibrary::Connect(Data::Receiver<IFontLibraryCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Connect(receiver);
}

void FontLibrary::Disconnect(Data::Receiver<IFontLibraryCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Disconnect(receiver);
}

FT_Library FontLibrary::GetFreeTypeLibrary() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetFreeTypeLibrary();
}

std::vector<Font> FontLibrary::GetFonts() const
{
    return GetImpl(m_impl_ptr).GetFonts();
}

bool FontLibrary::HasFont(const std::string& font_name) const
{
    return GetImpl(m_impl_ptr).HasFont(font_name);
}

Font& FontLibrary::GetFont(const std::string& font_name) const
{
    return GetImpl(m_impl_ptr).GetFont(font_name);
}

Font& FontLibrary::GetFont(const Data::IProvider& data_provider, const FontSettings& font_settings) const
{
    return GetImpl(m_impl_ptr).GetFont(data_provider, font_settings);
}

Font& FontLibrary::AddFont(const Data::IProvider& data_provider, const FontSettings& font_settings) const
{
    return GetImpl(m_impl_ptr).AddFont(data_provider, font_settings);
}

void FontLibrary::RemoveFont(const std::string& font_name) const
{
    GetImpl(m_impl_ptr).RemoveFont(font_name);
}

void FontLibrary::Clear() const
{
    GetImpl(m_impl_ptr).Clear();
}

FontContext::FontContext(const Data::IProvider& font_data_provider)
    : m_font_data_provider(font_data_provider)
{
}

FontContext::FontContext(const FontLibrary& font_lib, const Data::IProvider& font_data_provider)
    : m_font_lib(font_lib)
    , m_font_data_provider(font_data_provider)
{ }

Font& FontContext::GetFont(const FontSettings& font_settings) const
{
    return m_font_lib.GetFont(m_font_data_provider, font_settings);
}

} // namespace Methane::UserInterface
