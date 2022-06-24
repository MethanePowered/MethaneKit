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

#include <string_view>

namespace Methane::Graphics
{
struct RenderContext;
struct RenderCommandList;
struct RenderState;
struct RenderPattern;
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
    , protected Data::Receiver<IFontCallback> //NOSONAR
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
        Center,
        Justify
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

        [[nodiscard]]
        bool operator==(const Layout& other) const noexcept
        {
            return std::tie(wrap, horizontal_alignment, vertical_alignment) ==
                   std::tie(other.wrap, other.horizontal_alignment, other.vertical_alignment);
        }
    };

    template<typename StringType>
    struct Settings // NOSONAR
    {
        std::string name;
        StringType  text;
        UnitRect    rect;
        Layout      layout;
        Color4F     color { 1.F, 1.F, 1.F, 1.F };
        bool        incremental_update = true;
        bool        adjust_vertical_content_offset = true;

        // Minimize number of vertex/index buffer re-allocations on dynamic text updates by reserving additional size with multiplication of required size
        Data::Size  mesh_buffers_reservation_multiplier = 2U;

        // Text render state object name for using as a key in graphics object cache
        // NOTE: State name should be different in case of render state incompatibility between Text objects
        std::string state_name = "Screen Text Render State";

        Settings& SetName(std::string_view new_name) noexcept                                         { name = new_name; return *this; }
        Settings& SetText(const StringType& new_text) noexcept                                        { text = new_text; return *this; }
        Settings& SetRect(const UnitRect& new_rect) noexcept                                          { rect = new_rect; return *this; }
        Settings& SetLayout(const Layout& new_layout) noexcept                                        { layout = new_layout; return *this; }
        Settings& SetColor(const Color4F& new_color) noexcept                                         { color = new_color; return *this; }
        Settings& SetIncrementalUpdate(bool new_incremental_update) noexcept                          { incremental_update = new_incremental_update; return *this; }
        Settings& SetAdjustVerticalContentOffset(bool new_adjust_offset) noexcept                     { adjust_vertical_content_offset = new_adjust_offset; return *this; }
        Settings& SetMeshBuffersReservationMultiplier(Data::Size new_reservation_multiplier) noexcept { mesh_buffers_reservation_multiplier = new_reservation_multiplier; return *this; }
        Settings& SetStateName(std::string_view new_state_name) noexcept                            { state_name = new_state_name; return *this; }
    };

    using SettingsUtf8  = Settings<std::string>;
    using SettingsUtf32 = Settings<std::u32string>;


    Text(Context& ui_context, gfx::RenderPattern& render_pattern, Font& font, const SettingsUtf8& settings);
    Text(Context& ui_context, Font& font, const SettingsUtf8& settings);
    Text(Context& ui_context, gfx::RenderPattern& render_pattern, Font& font, SettingsUtf32 settings);
    Text(Context& ui_context, Font& font, SettingsUtf32 settings);
    ~Text() override;

    [[nodiscard]] const SettingsUtf32&  GetSettings() const noexcept  { return m_settings; }
    [[nodiscard]] const std::u32string& GetTextUtf32() const noexcept { return m_settings.text; }
    [[nodiscard]] std::string           GetTextUtf8() const;

    void SetText(std::string_view text);
    void SetText(std::u32string_view text);
    void SetTextInScreenRect(std::string_view text, const UnitRect& ui_rect);
    void SetTextInScreenRect(std::u32string_view text, const UnitRect& ui_rect);
    void SetColor(const gfx::Color4F& color);
    void SetLayout(const Layout& layout);
    void SetWrap(Wrap wrap);
    void SetHorizontalAlignment(HorizontalAlignment alignment);
    void SetVerticalAlignment(VerticalAlignment alignment);
    void SetIncrementalUpdate(bool incremental_update) noexcept { m_settings.incremental_update = incremental_update; }

    // Item overrides
    bool SetRect(const UnitRect& ui_rect) override;

    void Update(const gfx::FrameSize& render_attachment_size);
    void Draw(gfx::RenderCommandList& cmd_list, gfx::CommandList::DebugGroup* p_debug_group = nullptr);

protected:
    // IFontCallback interface
    void OnFontAtlasTextureReset(Font& font, const Ptr<gfx::Texture>& old_atlas_texture_ptr, const Ptr<gfx::Texture>& new_atlas_texture_ptr) override;
    void OnFontAtlasUpdated(Font&) override { /* not handled in this class */ }

private:
    struct CommonResourceRefs
    {
        gfx::RenderContext&      render_context;
        const gfx::RenderState&  render_state;
        const Ptr<gfx::Buffer>&  const_buffer_ptr;
        const Ptr<gfx::Texture>& atlas_texture_ptr;
        const Ptr<gfx::Sampler>& atlas_sampler_ptr;
        const TextMesh&          text_mesh;
    };

    class FrameResources
    {
    public:
        enum class DirtyFlags : uint32_t
        {
            None     = 0U,
            Mesh     = 1U << 0U,
            Uniforms = 1U << 1U,
            Atlas    = 1U << 2U,
            All      = Mesh | Uniforms | Atlas
        };

        FrameResources(uint32_t frame_index, const CommonResourceRefs& common_resources);

        void SetDirty(DirtyFlags dirty_flags) noexcept;

        [[nodiscard]] bool IsDirty(DirtyFlags dirty_flags) const noexcept;
        [[nodiscard]] bool IsDirty() const noexcept                           { return m_dirty_mask != DirtyFlags::None; }
        [[nodiscard]] bool IsInitialized() const noexcept                     { return m_program_bindings_ptr && m_vertex_buffer_set_ptr && m_index_buffer_ptr; }
        [[nodiscard]] bool IsAtlasInitialized() const noexcept                { return !!m_atlas_texture_ptr; }

        [[nodiscard]] gfx::BufferSet&       GetVertexBufferSet() const;
        [[nodiscard]] gfx::Buffer&          GetIndexBuffer() const;
        [[nodiscard]] gfx::ProgramBindings& GetProgramBindings() const;

        bool UpdateAtlasTexture(const Ptr<gfx::Texture>& new_atlas_texture_ptr); // returns true if probram bindings were updated, false if bindings have to be initialized
        void UpdateMeshBuffers(const gfx::RenderContext& render_context, const TextMesh& text_mesh, std::string_view text_name, Data::Size reservation_multiplier);
        void UpdateUniformsBuffer(const gfx::RenderContext& render_context, const TextMesh& text_mesh, std::string_view text_name);
        void InitializeProgramBindings(const gfx::RenderState& state, const Ptr<gfx::Buffer>& const_buffer_ptr,
                                       const Ptr<gfx::Sampler>& atlas_sampler_ptr, std::string_view text_name);

    private:
        uint32_t                  m_frame_index;
        DirtyFlags                m_dirty_mask = DirtyFlags::All;
        Ptr<gfx::BufferSet>       m_vertex_buffer_set_ptr;
        Ptr<gfx::Buffer>          m_index_buffer_ptr;
        Ptr<gfx::Buffer>          m_uniforms_buffer_ptr;
        Ptr<gfx::Texture>         m_atlas_texture_ptr;
        Ptr<gfx::ProgramBindings> m_program_bindings_ptr;
    };

    void InitializeFrameResources();
    void MakeFrameResourcesDirty(FrameResources::DirtyFlags dirty_flags);
    FrameResources& GetCurrentFrameResources();

    void UpdateTextMesh();
    void UpdateConstantsBuffer();

    struct UpdateRectResult
    {
        bool rect_changed = false;
        bool size_changed = false;
    };

    UpdateRectResult UpdateRect(const UnitRect& ui_rect, bool reset_content_rect);
    FrameRect GetAlignedViewportRect() const;
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
    bool                        m_is_viewport_dirty = true;
    bool                        m_is_const_buffer_dirty = true;
};

} // namespace Methane::Graphics
