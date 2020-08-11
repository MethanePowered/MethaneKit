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

    enum class HorizontalAlignment : uint32_t
    {
        Left = 0u,
        Right,
        Center
    };

    enum class VerticalAlignment : uint32_t
    {
        Top = 0u,
        Bottom,
        Center
    };

    struct Layout
    {
        Wrap                wrap                 = Wrap::Anywhere;
        HorizontalAlignment horizontal_alignment = HorizontalAlignment::Left;
        VerticalAlignment   vertical_alignment   = VerticalAlignment::Top;

        bool operator==(const Layout& other) const noexcept
        {
            return std::tie(wrap, horizontal_alignment, vertical_alignment) ==
                   std::tie(other.wrap, other.horizontal_alignment, other.vertical_alignment);
        }
    };

    template<typename StringType>
    struct Settings
    {
        const std::string   name;
        StringType          text;
        UnitRect            rect;
        Layout              layout;
        Color4f             color                { 1.f, 1.f, 1.f, 1.f };
        bool                incremental_update   = true;

        // Minimize number of vertex/index buffer re-allocations on dynamic text updates by reserving additional size with multiplication of required size
        Data::Size        mesh_buffers_reservation_multiplier = 2u;
    };

    using SettingsUtf8  = Settings<std::string>;
    using SettingsUtf32 = Settings<std::u32string>;

    Text(Context& ui_context, Font& font, const SettingsUtf8&  settings);
    Text(Context& ui_context, Font& font, SettingsUtf32 settings);
    ~Text();

    const SettingsUtf32&  GetSettings() const noexcept            { return m_settings; }
    const std::u32string& GetTextUtf32() const noexcept           { return m_settings.text; }
    std::string           GetTextUtf8() const;

    void SetText(const std::string& text);
    void SetText(const std::u32string& text);
    void SetTextInScreenRect(const std::string& text, const UnitRect& ui_rect);
    void SetTextInScreenRect(const std::u32string& text, const UnitRect& ui_rect);
    void SetColor(const gfx::Color4f& color);
    void SetLayout(const Layout& layout);
    void SetWrap(Wrap wrap);
    void SetHorizontalAlignment(HorizontalAlignment alignment);
    void SetVerticalAlignment(VerticalAlignment alignment);
    void SetIncrementalUpdate(bool incremental_update) noexcept { m_settings.incremental_update = incremental_update; }

    // Item overrides
    bool SetRect(const UnitRect& ui_rect) override;

    void Update();
    void Draw(gfx::RenderCommandList& cmd_list);

    static std::string GetWrapName(Wrap wrap) noexcept;
    static std::string GetHorizontalAlignmentName(HorizontalAlignment alignment) noexcept;
    static std::string GetVerticalAlignmentName(VerticalAlignment alignment) noexcept;

protected:
    // IFontCallback interface
    void OnFontAtlasTextureReset(Font& font, const Ptr<gfx::Texture>& sp_old_atlas_texture, const Ptr<gfx::Texture>& sp_new_atlas_texture) override;
    void OnFontAtlasUpdated(Font&) override {}

private:
    struct Constants;
    struct Uniforms;

    struct Dirty
    {
        using Mask = uint32_t;
        enum Value : Mask
        {
            None         = 0u,
            Mesh         = 1u << 0u,
            Uniforms     = 1u << 1u,
            Atlas        = 1u << 2u,
            All          = ~0u,
        };
    };

    class FrameResources
    {
    public:
        FrameResources(gfx::RenderState& state, gfx::RenderContext& render_context,
                       const Ptr<gfx::Buffer>& sp_const_buffer, const Ptr<gfx::Texture>& sp_atlas_texture, const Ptr<gfx::Sampler>& sp_atlas_sampler,
                       const TextMesh& text_mesh, const std::string& text_name, Data::Size reservation_multiplier);

        void SetDirty(Dirty::Mask dirty_flags) noexcept         { m_dirty_mask |= dirty_flags; }
        bool IsDirty() const noexcept                           { return m_dirty_mask != Dirty::None; }
        bool IsDirty(Dirty::Mask dirty_flags) const noexcept    { return m_dirty_mask & dirty_flags; }
        bool IsInitialized() const noexcept                     { return m_sp_program_bindings && m_sp_vertex_buffer_set && m_sp_index_buffer; }
        bool IsAtlasInitialized() const noexcept                { return !!m_sp_atlas_texture; }

        gfx::BufferSet&       GetVertexBufferSet() const;
        gfx::Buffer&          GetIndexBuffer() const;
        gfx::ProgramBindings& GetProgramBindings() const;

        bool UpdateAtlasTexture(const Ptr<gfx::Texture>& sp_new_atlas_texture); // returns true if probram bindings were updated, false if bindings have to be initialized
        void UpdateMeshBuffers(gfx::RenderContext& render_context, const TextMesh& text_mesh, const std::string& text_name, Data::Size reservation_multiplier);
        void UpdateUniformsBuffer(gfx::RenderContext& render_context, const TextMesh& text_mesh, const std::string& text_name);
        void InitializeProgramBindings(gfx::RenderState& state, const Ptr<gfx::Buffer>& sp_const_buffer, const Ptr<gfx::Sampler>& sp_atlas_sampler);

    private:
        Dirty::Mask               m_dirty_mask = Dirty::None;
        Ptr<gfx::BufferSet>       m_sp_vertex_buffer_set;
        Ptr<gfx::Buffer>          m_sp_index_buffer;
        Ptr<gfx::Buffer>          m_sp_uniforms_buffer;
        Ptr<gfx::Texture>         m_sp_atlas_texture;
        Ptr<gfx::ProgramBindings> m_sp_program_bindings;
    };

    void InitializeFrameResources();
    void MakeFrameResourcesDirty(Dirty::Mask dirty_flags);
    FrameResources& GetCurrentFrameResources();

    void UpdateTextMesh();
    void UpdateConstantsBuffer();

    struct UpdateRectResult
    {
        bool rect_changed = false;
        bool size_changed = false;
    };

    UpdateRectResult UpdateRect(const UnitRect& ui_rect, bool reset_content_rect);
    FrameRect GetAlignedViewportRect();
    bool UpdateViewportAndItemRect(Units ui_rect_units);

    SettingsUtf32               m_settings;
    UnitRect                    m_frame_rect;
    Ptr<Font>                   m_sp_font;
    UniquePtr<TextMesh>         m_sp_text_mesh;
    Ptr<gfx::RenderState>       m_sp_state;
    Ptr<gfx::Buffer>            m_sp_const_buffer;
    Ptr<gfx::Sampler>           m_sp_atlas_sampler;
    std::vector<FrameResources> m_frame_resources;
    bool                        m_is_rendering = false;
};

} // namespace Methane::Graphics
