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

#include "Font.h"

#include <Methane/UserInterface/Item.h>
#include <Methane/Graphics/Color.hpp>
#include <Methane/Data/Receiver.hpp>

namespace Methane::Graphics
{
struct RenderContext;
struct RenderCommandList;
struct RenderState;
struct ProgramBindings;
struct Buffer;
struct BufferSet;
struct Texture;
struct Sampler;
}

namespace Methane::UserInterface
{

class TextMesh;

class Text
    : public Item
    , protected Data::Receiver<IFontCallback>
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
        UnitRect          rect;
        Color4f           color              { 1.f, 1.f, 1.f, 1.f };
        Wrap              wrap               = Wrap::Anywhere;
        bool              incremental_update = true;

        // Minimize number of vertex/index buffer re-allocations on dynamic text updates by reserving additional size with multiplication of required size
        Data::Size        mesh_buffers_reservation_multiplier = 2u;
    };

    using SettingsUtf8  = Settings<std::string>;
    using SettingsUtf32 = Settings<std::u32string>;

    Text(Context& ui_context, Font& font, const SettingsUtf8&  settings);
    Text(Context& ui_context, Font& font, SettingsUtf32 settings);
    ~Text();

    const SettingsUtf32&  GetSettings() const noexcept         { return m_settings; }
    const gfx::FrameRect& GetViewport() const noexcept         { return m_viewport_rect; }
    const std::u32string& GetTextUtf32() const noexcept        { return m_settings.text; }
    std::string           GetTextUtf8() const;
    const UnitRect&       GetViewportInPixels() const noexcept { return m_viewport_rect; }
    UnitRect              GetViewportInDots() const;

    void SetText(const std::string& text);
    void SetText(const std::u32string& text);
    void SetTextInScreenRect(const std::string& text, const UnitRect& ui_rect);
    void SetTextInScreenRect(const std::u32string& text, const UnitRect& ui_rect);
    void SetColor(const gfx::Color4f& color);
    void SetIncrementalUpdate(bool incremental_update) noexcept { m_settings.incremental_update = incremental_update; }

    // Item overrides
    bool SetRect(const UnitRect& ui_rect) override;

    void Draw(gfx::RenderCommandList& cmd_list);

protected:
    // IFontCallback interface
    void OnFontAtlasTextureReset(Font& font, const Ptr<gfx::Texture>& sp_old_atlas_texture, const Ptr<gfx::Texture>& sp_new_atlas_texture) override;
    void OnFontAtlasUpdated(Font&) override {}

private:
    struct Constants;
    struct Uniforms;

    Ptr<gfx::ProgramBindings> CreateProgramBindings();

    void UpdateAtlasTexture(const Ptr<gfx::Texture>& sp_new_atlas_texture);
    void UpdateMeshData();
    void UpdateUniformsBuffer();
    void UpdateConstantsBuffer();

    SettingsUtf32             m_settings;
    UnitRect                  m_viewport_rect;
    Ptr<Font>                 m_sp_font;
    UniquePtr<TextMesh>       m_sp_text_mesh;
    Ptr<gfx::RenderState>     m_sp_state;
    Ptr<gfx::BufferSet>       m_sp_vertex_buffer_set;
    Ptr<gfx::Buffer>          m_sp_index_buffer;
    Ptr<gfx::Buffer>          m_sp_const_buffer;
    Ptr<gfx::Buffer>          m_sp_uniforms_buffer;
    Ptr<gfx::Texture>         m_sp_atlas_texture;
    Ptr<gfx::Sampler>         m_sp_texture_sampler;
    Ptr<gfx::ProgramBindings> m_sp_curr_program_bindings;
    Ptr<gfx::ProgramBindings> m_sp_prev_program_bindings;
    Data::Index               m_prev_program_bindings_release_on_frame  = 0u;
};

} // namespace Methane::Graphics