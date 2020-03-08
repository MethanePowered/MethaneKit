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

FILE: Methane/Graphics/Font.h
Font atlas textures generation and fonts library management classes.

******************************************************************************/

#pragma once

#include <Methane/Graphics/RenderContext.h>
#include <Methane/Graphics/Texture.h>
#include <Methane/Graphics/Types.h>

#include <map>
#include <string>

namespace Methane::Graphics
{

class Font
{
    friend class Library;
public:
    struct Settings
    {
        const std::string  name;
    };

    class Library
    {
        friend class Font;

    public:
        static Library& Get();

        Ptr<Font>     AddFont(Settings font_settings);
        WeakPtr<Font> GetFont(std::string font_name);

    protected:
        class Impl;
        Impl& GetImpl() { return *m_sp_impl; }

    private:
        Library();

        using FontByName = std::map<std::string, WeakPtr<Font>>;

        const UniquePtr<Impl> m_sp_impl;
        FontByName            m_font_by_name;
    };

    Texture& GetAtlasTexture(Context& context);
    void RemoveAtlasTexture(Context& context);

private:
    Font(Settings settings); // Font can be created with Font::Library::CreateFont

    using TextureByContext = std::map<Context*, Ptr<Texture>>;

    const Settings   m_settings;
    TextureByContext m_atlas_textures;
};

} // namespace Methane::Graphics
