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

#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/RenderContext.h>
#include <Methane/Graphics/RHI/RenderCommandList.h>
#include <Methane/Graphics/RHI/RenderPass.h>
#include <Methane/Graphics/RHI/RenderState.h>
#include <Methane/Graphics/RHI/ViewState.h>
#include <Methane/Graphics/RHI/Buffer.h>
#include <Methane/Graphics/RHI/Texture.h>
#include <Methane/Graphics/RHI/Sampler.h>
#include <Methane/Graphics/RHI/ProgramBindings.h>
#include <Methane/Graphics/Types.h>
#include <Methane/Graphics/Rect.hpp>
#include <Methane/Graphics/Color.hpp>
#include <Methane/Memory.hpp>

namespace Methane::Graphics
{

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

    ScreenQuad(const Rhi::CommandQueue& render_cmd_queue, const Rhi::RenderPattern& render_pattern, const Settings& settings);
    ScreenQuad(const Rhi::CommandQueue& render_cmd_queue, const Rhi::RenderPattern& render_pattern, const Rhi::Texture& texture, const Settings& settings);
    virtual ~ScreenQuad() = default;

    void SetBlendColor(const Color4F& blend_color);
    void SetScreenRect(const FrameRect& screen_rect, const FrameSize& render_attachment_size);
    void SetAlphaBlendingEnabled(bool alpha_blending_enabled);
    void SetTexture(Rhi::Texture texture);

    [[nodiscard]] const Settings& GetQuadSettings() const noexcept { return m_settings; }
    [[nodiscard]] const Rhi::Texture& GetTexture() const noexcept  { return m_texture; }

    virtual void Draw(const Rhi::RenderCommandList& cmd_list, const Rhi::CommandListDebugGroup* debug_group_ptr = nullptr) const;

protected:
    const Rhi::RenderPattern& GetRenderPattern() const noexcept { return m_render_pattern; }

private:
    void UpdateConstantsBuffer() const;

    [[nodiscard]] static Rhi::IShader::MacroDefinitions GetPixelShaderMacroDefinitions(TextureMode texture_mode);

    Settings                 m_settings;
    const Rhi::CommandQueue  m_render_cmd_queue;
    const Rhi::RenderPattern m_render_pattern;
    Rhi::RenderState         m_render_state;
    Rhi::ViewState           m_view_state;
    Rhi::BufferSet           m_vertex_buffer_set;
    Rhi::Buffer              m_index_buffer;
    Rhi::Buffer              m_const_buffer;
    Rhi::Texture             m_texture;
    Rhi::Sampler             m_texture_sampler;
    Rhi::ProgramBindings     m_const_program_bindings;
};

} // namespace Methane::Graphics
