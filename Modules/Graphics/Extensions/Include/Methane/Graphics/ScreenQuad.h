/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/ScreenQuad.h
ScreenQuad rendering primitive.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Types.h>
#include <Methane/Graphics/Rect.hpp>
#include <Methane/Graphics/Color.hpp>
#include <Methane/Graphics/CommandList.h>
#include <Methane/Graphics/IShader.h>
#include <Methane/Memory.hpp>

namespace Methane::Graphics
{

struct CommandQueue;
struct IRenderContext;
struct RenderCommandList;
struct RenderPattern;
struct RenderState;
struct ViewState;
struct BufferSet;
struct Buffer;
struct Texture;
struct Sampler;
struct ProgramBindings;

class ScreenQuad
{
public:
    enum class TextureMode : uint32_t
    {
        Disabled = 0U,
        RgbaFloat,
        RFloatToAlpha,
    };

    struct Settings
    {
        const std::string name;
        FrameRect         screen_rect;
        bool              alpha_blending_enabled = false;
        Color4F           blend_color            { 1.F, 1.F, 1.F, 1.F };
        TextureMode       texture_mode           = TextureMode::RgbaFloat;
    };

    ScreenQuad(CommandQueue& render_cmd_queue, RenderPattern& render_pattern, const Settings& settings);
    ScreenQuad(CommandQueue& render_cmd_queue, RenderPattern& render_pattern, const Ptr<Texture>& texture_ptr, const Settings& settings);
    virtual ~ScreenQuad() = default;

    void SetBlendColor(const Color4F& blend_color);
    void SetScreenRect(const FrameRect& screen_rect, const FrameSize& render_attachment_size);
    void SetAlphaBlendingEnabled(bool alpha_blending_enabled);
    void SetTexture(Ptr<Texture> texture_ptr);

    [[nodiscard]] const Settings& GetQuadSettings() const noexcept     { return m_settings; }
    [[nodiscard]] const Texture&  GetTexture() const;

    virtual void Draw(RenderCommandList& cmd_list, CommandList::DebugGroup* p_debug_group = nullptr) const;

protected:
    RenderPattern& GetRenderPattern() const noexcept { return *m_render_pattern_ptr; }
    const IRenderContext& GetRenderContext() const noexcept;

private:
    void UpdateConstantsBuffer() const;

    [[nodiscard]] static IShader::MacroDefinitions GetPixelShaderMacroDefinitions(TextureMode texture_mode);

    Settings                 m_settings;
    const Ptr<CommandQueue>  m_render_cmd_queue_ptr;
    const Ptr<RenderPattern> m_render_pattern_ptr;
    Ptr<RenderState>         m_render_state_ptr;
    Ptr<ViewState>           m_view_state_ptr;
    Ptr<BufferSet>           m_vertex_buffer_set_ptr;
    Ptr<Buffer>              m_index_buffer_ptr;
    Ptr<Buffer>              m_const_buffer_ptr;
    Ptr<Texture>             m_texture_ptr;
    Ptr<Sampler>             m_texture_sampler_ptr;
    Ptr<ProgramBindings>     m_const_program_bindings_ptr;
};

} // namespace Methane::Graphics
