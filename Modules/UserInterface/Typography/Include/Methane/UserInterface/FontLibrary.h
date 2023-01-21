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

#pragma once

#include "Font.h"

#include <Methane/Pimpl.h>

#ifndef FT_Library
typedef struct FT_LibraryRec_* FT_Library; // NOSONAR
#endif

namespace Methane::UserInterface
{

struct IFontLibraryCallback
{
    virtual void OnFontAdded(Font& font) = 0;
    virtual void OnFontRemoved(Font& font) = 0;

    virtual ~IFontLibraryCallback() = default;
};

class FontLibrary
{
    friend class Font;

public:
    FontLibrary();

    void Connect(Data::Receiver<IFontLibraryCallback>& receiver) const;
    void Disconnect(Data::Receiver<IFontLibraryCallback>& receiver) const;

    [[nodiscard]] FT_Library GetFreeTypeLibrary() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] std::vector<Font> GetFonts() const;
    [[nodiscard]] bool HasFont(const std::string& font_name) const;
    [[nodiscard]] Font& GetFont(const std::string& font_name) const;
    [[nodiscard]] Font& GetFont(const Data::IProvider& data_provider, const FontSettings& font_settings) const;
    Font& AddFont(const Data::IProvider& data_provider, const FontSettings& font_settings) const;
    void RemoveFont(const std::string& font_name) const;
    void Clear() const;

private:
    class Impl;

    const Ptr<Impl> m_impl_ptr;
};

} // namespace Methane::UserInterface
