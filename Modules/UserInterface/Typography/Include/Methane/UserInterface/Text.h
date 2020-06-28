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

FILE: Methane/UserInterface/Text.h
Methane text rendering primitive.

******************************************************************************/

#pragma once

#include <Methane/UserInterface/Font.h>
#include <Methane/Graphics/RenderContext.h>
#include <Methane/Graphics/Texture.h>
#include <Methane/Graphics/Buffer.h>
#include <Methane/Graphics/RenderState.h>
#include <Methane/Graphics/Program.h>
#include <Methane/Graphics/ProgramBindings.h>
#include <Methane/Graphics/Sampler.h>
#include <Methane/Graphics/Types.h>
#include <Methane/Data/Receiver.hpp>

namespace Methane::Graphics
{

struct RenderCommandList;

class Text : protected Data::Receiver<IFontCallback>
{
public:
    enum class Wrap : uint32_t
    {
        None = 0u,
        Anywhere,
        Word
    };

    template<typename StringType>
    struct Settings
    {
        const std::string name;
        StringType        text;
        FrameRect         screen_rect;
        bool              screen_rect_in_pixels = false;
        Color4f           color = Color4f(1.f, 1.f, 1.f, 1.f);
        Wrap              wrap  = Wrap::Anywhere;
    };

    using SettingsUtf8  = Settings<std::string>;
    using SettingsUtf32 = Settings<std::u32string>;

    Text(RenderContext& context, Font& font, const SettingsUtf8&  settings);
    Text(RenderContext& context, Font& font, SettingsUtf32 settings);
    ~Text();

    const SettingsUtf32&  GetSettings() const noexcept       { return m_settings; }
    const FrameRect&      GetViewport() const noexcept       { return m_viewport_rect; }
    FrameRect             GetViewportInDots() const noexcept { return m_viewport_rect / m_context.GetContentScalingFactor(); }
    const std::u32string& GetTextUtf32() const noexcept      { return m_settings.text; }
    std::string           GetTextUtf8() const;

    void SetText(const std::string& text);
    void SetText(const std::u32string& text);
    void SetTextInScreenRect(const std::string& text, const FrameRect& screen_rect, bool rect_in_pixels = false);
    void SetTextInScreenRect(const std::u32string& text, const FrameRect& screen_rect, bool rect_in_pixels = false);
    void SetScreenRect(const FrameRect& screen_rect, bool rect_in_pixels = false);
    void SetColor(const Color4f& color);

    void Draw(RenderCommandList& cmd_list);

protected:
    // IFontCallback interface
    void OnFontAtlasTextureReset(Font& font, const Ptr<Texture>& sp_old_atlas_texture, const Ptr<Texture>& sp_new_atlas_texture) override;
    void OnFontAtlasUpdated(Font&) override {}

private:
    struct Mesh;
    struct Constants;

    Ptr<ProgramBindings> CreateConstProgramBindings();

    void UpdateAtlasTexture();
    void UpdateMeshData();
    void UpdateMeshBuffers();
    void UpdateConstantsBuffer();

    SettingsUtf32        m_settings;
    FrameRect            m_viewport_rect;
    RenderContext&       m_context;
    Ptr<Font>            m_sp_font;
    Ptr<RenderState> m_sp_state;
    Ptr<BufferSet>   m_sp_vertex_buffers;
    Ptr<Buffer>      m_sp_index_buffer;
    Ptr<Buffer>          m_sp_const_buffer;
    Ptr<Texture>         m_sp_atlas_texture;
    Ptr<Texture>         m_sp_new_atlas_texture;
    Ptr<Sampler>         m_sp_texture_sampler;
    Ptr<ProgramBindings> m_sp_const_program_bindings;
    UniquePtr<Mesh>      m_sp_new_mesh_data;
    UniquePtr<Constants> m_sp_new_const_data;
};

} // namespace Methane::Graphics
