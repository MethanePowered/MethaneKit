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

#include <Methane/Graphics/RenderContext.h>
#include <Methane/Graphics/CommandList.h>
#include <Methane/Graphics/Texture.h>
#include <Methane/Graphics/Buffer.h>
#include <Methane/Graphics/RenderState.h>
#include <Methane/Graphics/Program.h>
#include <Methane/Graphics/ProgramBindings.h>
#include <Methane/Graphics/Sampler.h>
#include <Methane/Graphics/Types.h>

#include <memory>

namespace Methane::Graphics
{

struct RenderCommandList;

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
        Color4f           blend_color            { 1.F, 1.F, 1.F, 1.F };
        TextureMode       texture_mode           = TextureMode::RgbaFloat;
    };

    ScreenQuad(RenderContext& context, Settings settings);
    ScreenQuad(RenderContext& context, Ptr<Texture> texture_ptr, Settings settings);

    void SetBlendColor(const Color4f& blend_color);
    void SetScreenRect(const FrameRect& screen_rect, const FrameSize& render_attachment_size);
    void SetAlphaBlendingEnabled(bool alpha_blending_enabled);
    void SetTexture(Ptr<Texture> texture_ptr);

    const Settings& GetSettings() const noexcept { return m_settings; }
    FrameRect       GetScreenRectInDots() const noexcept { return m_settings.screen_rect / m_context.GetContentScalingFactor(); }
    const Texture&  GetTexture() const;

    void Draw(RenderCommandList& cmd_list, CommandList::DebugGroup* p_debug_group = nullptr) const;

protected:
    RenderContext& GetRenderContext() noexcept { return m_context; }

private:
    void UpdateConstantsBuffer() const;

    static Shader::MacroDefinitions GetPixelShaderMacroDefinitions(TextureMode texture_mode);

    Settings             m_settings;
    RenderContext&       m_context;
    Ptr<RenderState>     m_render_state_ptr;
    Ptr<ViewState>       m_view_state_ptr;
    Ptr<BufferSet>       m_vertex_buffer_set_ptr;
    Ptr<Buffer>          m_index_buffer_ptr;
    Ptr<Buffer>          m_const_buffer_ptr;
    Ptr<Texture>         m_texture_ptr;
    Ptr<Sampler>         m_texture_sampler_ptr;
    Ptr<ProgramBindings> m_const_program_bindings_ptr;
};

} // namespace Methane::Graphics
