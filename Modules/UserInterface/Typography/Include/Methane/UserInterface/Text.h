/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/UserInterface/Text.h
Methane text rendering primitive.

******************************************************************************/

#pragma once

#include "Font.h"

#include <Methane/UserInterface/Item.h>
#include <Methane/Graphics/CommandList.h>
#include <Methane/Graphics/Color.hpp>
#include <Methane/Data/Receiver.hpp>

namespace Methane::Graphics
{
struct RenderContext;
struct RenderCommandList;
struct RenderState;
struct ViewState;
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
        None = 0U,
        Anywhere,
        Word
    };

    enum class HorizontalAlignment : uint32_t
    {
        Left = 0U,
        Right,
        Center
    };

    enum class VerticalAlignment : uint32_t
    {
        Top = 0U,
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
        const std::string name;
        StringType        text;
        UnitRect          rect;
        Layout            layout;
        Color4f           color                { 1.F, 1.F, 1.F, 1.F };
        bool              incremental_update   = true;
        bool              adjust_vertical_content_offset = true;

        // Minimize number of vertex/index buffer re-allocations on dynamic text updates by reserving additional size with multiplication of required size
        Data::Size        mesh_buffers_reservation_multiplier = 2U;
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

    void Update(const gfx::FrameSize& render_attachment_size);
    void Draw(gfx::RenderCommandList& cmd_list, gfx::CommandList::DebugGroup* p_debug_group = nullptr);

    static std::string GetWrapName(Wrap wrap);
    static std::string GetHorizontalAlignmentName(HorizontalAlignment alignment);
    static std::string GetVerticalAlignmentName(VerticalAlignment alignment);

protected:
    // IFontCallback interface
    void OnFontAtlasTextureReset(Font& font, const Ptr<gfx::Texture>& old_atlas_texture_ptr, const Ptr<gfx::Texture>& new_atlas_texture_ptr) override;
    void OnFontAtlasUpdated(Font&) override {}

private:
    struct Constants;
    struct Uniforms;

    class FrameResources
    {
    public:
        struct Dirty
        {
            using Mask = uint32_t;
            enum Value : Mask
            {
                None         = 0U,
                Mesh         = 1U << 0U,
                Uniforms     = 1U << 1U,
                Atlas        = 1U << 2U,
                All          = ~0U,
            };
        };

        FrameResources(gfx::RenderState& state, gfx::RenderContext& render_context,
                       const Ptr<gfx::Buffer>& const_buffer_ptr, const Ptr<gfx::Texture>& atlas_texture_ptr, const Ptr<gfx::Sampler>& atlas_sampler_ptr,
                       const TextMesh& text_mesh, const std::string& text_name, Data::Size reservation_multiplier);

        void SetDirty(Dirty::Mask dirty_flags) noexcept         { m_dirty_mask |= dirty_flags; }
        bool IsDirty() const noexcept                           { return m_dirty_mask != Dirty::None; }
        bool IsDirty(Dirty::Mask dirty_flags) const noexcept    { return m_dirty_mask & dirty_flags; }
        bool IsInitialized() const noexcept                     { return m_program_bindings_ptr && m_vertex_buffer_set_ptr && m_index_buffer_ptr; }
        bool IsAtlasInitialized() const noexcept                { return !!m_atlas_texture_ptr; }

        gfx::BufferSet&       GetVertexBufferSet() const;
        gfx::Buffer&          GetIndexBuffer() const;
        gfx::ProgramBindings& GetProgramBindings() const;

        bool UpdateAtlasTexture(const Ptr<gfx::Texture>& new_atlas_texture_ptr); // returns true if probram bindings were updated, false if bindings have to be initialized
        void UpdateMeshBuffers(gfx::RenderContext& render_context, const TextMesh& text_mesh, const std::string& text_name, Data::Size reservation_multiplier);
        void UpdateUniformsBuffer(gfx::RenderContext& render_context, const TextMesh& text_mesh, const std::string& text_name);
        void InitializeProgramBindings(gfx::RenderState& state, const Ptr<gfx::Buffer>& const_buffer_ptr, const Ptr<gfx::Sampler>& atlas_sampler_ptr);

    private:
        Dirty::Mask               m_dirty_mask = Dirty::None;
        Ptr<gfx::BufferSet>       m_vertex_buffer_set_ptr;
        Ptr<gfx::Buffer>          m_index_buffer_ptr;
        Ptr<gfx::Buffer>          m_uniforms_buffer_ptr;
        Ptr<gfx::Texture>         m_atlas_texture_ptr;
        Ptr<gfx::ProgramBindings> m_program_bindings_ptr;
    };

    void InitializeFrameResources();
    void MakeFrameResourcesDirty(FrameResources::Dirty::Mask dirty_flags);
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
    void UpdateViewport(const gfx::FrameSize& render_attachment_size);

    SettingsUtf32               m_settings;
    UnitRect                    m_frame_rect;
    FrameSize                   m_render_attachment_size = FrameSize::Max();
    Ptr<Font>                   m_font_ptr;
    UniquePtr<TextMesh>         m_text_mesh_ptr;
    Ptr<gfx::RenderState>       m_render_state_ptr;
    Ptr<gfx::ViewState>         m_view_state_ptr;
    Ptr<gfx::Buffer>            m_const_buffer_ptr;
    Ptr<gfx::Sampler>           m_atlas_sampler_ptr;
    std::vector<FrameResources> m_frame_resources;
    bool                        m_is_viewport_dirty;
};

} // namespace Methane::Graphics
