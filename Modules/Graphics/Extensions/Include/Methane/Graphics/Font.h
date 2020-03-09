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
#include <Methane/Data/Provider.h>

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
        std::string name;
        std::string font_path;
        uint32_t    font_size_pt;
        uint32_t    resolution_dpi;
        std::string letters;
    };

    class Library
    {
        friend class Font;

    public:
        static Library& Get();

        const Ptr<Font>& Add(const Data::Provider& data_provider, const Settings& font_settings);
        bool  Has(const std::string& font_name) const;
        Font& Get(const std::string& font_name) const;
        void  Remove(const std::string& font_name);
        void  Clear();

    protected:
        class Impl;
        Impl& GetImpl() { return *m_sp_impl; }

    private:
        Library();

        using FontByName = std::map<std::string, Ptr<Font>>;

        const UniquePtr<Impl> m_sp_impl;
        FontByName            m_font_by_name;
    };

    const Settings& GetSettings() const { return m_settings; }

    Texture& GetAtlasTexture(Context& context);
    void     RemoveAtlasTexture(Context& context);

protected:
    Font(const Data::Provider& data_provider, const Settings& settings); // Font can be created only via Font::Library::Add

private:
    using TextureByContext = std::map<Context*, Ptr<Texture>>;

    Settings         m_settings;
    TextureByContext m_atlas_textures;
};

} // namespace Methane::Graphics
