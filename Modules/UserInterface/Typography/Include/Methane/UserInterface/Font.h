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

FILE: Methane/UserInterface/Font.h
Font atlas texture generation class.

******************************************************************************/

#pragma once

#include <Methane/Pimpl.h>
#include <Methane/Graphics/Rect.hpp>
#include <Methane/Data/IProvider.h>
#include <Methane/Data/Receiver.hpp>

#include <cstdint>
#include <stdexcept>
#include <string>

namespace Methane::Graphics::Rhi
{
    class RenderContext;
    class Texture;
}

namespace Methane::UserInterface
{

namespace gfx = Methane::Graphics;
namespace rhi = Methane::Graphics::Rhi;

struct FontDescription
{
    std::string name;
    std::string path;
    uint32_t    size_pt;
};

struct FontSettings
{
    FontDescription description;
    uint32_t        resolution_dpi;
    std::u32string  characters;
};

class FreeTypeError
    : public std::runtime_error
{
public:
    using ErrorCode = int;

    explicit FreeTypeError(ErrorCode error);

    [[nodiscard]] ErrorCode GetError() const noexcept { return m_error; }

private:
    const ErrorCode m_error;
};

class Font;

struct IFontCallback
{
    virtual void OnFontAtlasTextureReset(Font& font, const rhi::Texture* old_atlas_texture_ptr, const rhi::Texture* new_atlas_texture_ptr) = 0;
    virtual void OnFontAtlasUpdated(Font& font) = 0;

    virtual ~IFontCallback() = default;
};

class FontLibrary;

class Font
{
public:
    class Impl;

    using Description = FontDescription;
    using Settings    = FontSettings;
    using Library     = FontLibrary;

    [[nodiscard]] static std::u32string ConvertUtf8To32(std::string_view text);
    [[nodiscard]] static std::string    ConvertUtf32To8(std::u32string_view text);
    [[nodiscard]] static std::u32string GetAlphabetDefault() { return GetAlphabetInRange(32, 126); }
    [[nodiscard]] static std::u32string GetAlphabetInRange(char32_t from, char32_t to);
    [[nodiscard]] static std::u32string GetAlphabetFromText(const std::string& text);
    [[nodiscard]] static std::u32string GetAlphabetFromText(const std::u32string& text);

    META_PIMPL_METHODS_DECLARE_NO_INLINE(Font);
    META_PIMPL_METHODS_COMPARE_DECLARE_NO_INLINE(Font);

    Font(const Library& font_lib, const Data::IProvider& data_provider, const Settings& settings);

    [[nodiscard]] const Settings& GetSettings() const META_PIMPL_NOEXCEPT;

    void Connect(Data::Receiver<IFontCallback>& receiver) const;
    void Disconnect(Data::Receiver<IFontCallback>& receiver) const;

    void ResetChars(const std::string& utf8_characters) const;
    void ResetChars(const std::u32string& utf32_characters) const;
    void AddChars(const std::string& utf8_characters) const;
    void AddChars(const std::u32string& utf32_characters) const;
    void AddChar(char32_t char_code) const;

    [[nodiscard]] uint32_t GetLineHeight() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] const gfx::FrameSize& GetMaxGlyphSize() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] const gfx::FrameSize& GetAtlasSize() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] const rhi::Texture&   GetAtlasTexture(const rhi::RenderContext& context) const;

    void RemoveAtlasTexture(const rhi::RenderContext& render_context) const;
    void ClearAtlasTextures() const;

    long GetUseCount() const { return m_impl_ptr.use_count(); }
    Impl& GetImplementation();
    const Impl& GetImplementation() const;

private:
    Ptr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics
