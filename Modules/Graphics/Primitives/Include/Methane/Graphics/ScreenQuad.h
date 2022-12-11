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
#include <Methane/Graphics/RHI/ICommandList.h>
#include <Methane/Graphics/RHI/IShader.h>
#include <Methane/Memory.hpp>

namespace Methane::Graphics::Rhi
{

struct ICommandQueue;
struct IRenderContext;
struct IRenderCommandList;
struct IRenderPattern;
struct IRenderState;
struct IViewState;
struct IBufferSet;
struct IBuffer;
struct ITexture;
struct ISampler;
struct IProgramBindings;

} // namespace Methane::Graphics::Rhi

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

    ScreenQuad(Rhi::ICommandQueue& render_cmd_queue, Rhi::IRenderPattern& render_pattern, const Settings& settings);
    ScreenQuad(Rhi::ICommandQueue& render_cmd_queue, Rhi::IRenderPattern& render_pattern, const Ptr<Rhi::ITexture>& texture_ptr, const Settings& settings);
    virtual ~ScreenQuad() = default;

    void SetBlendColor(const Color4F& blend_color);
    void SetScreenRect(const FrameRect& screen_rect, const FrameSize& render_attachment_size);
    void SetAlphaBlendingEnabled(bool alpha_blending_enabled);
    void SetTexture(Ptr<Rhi::ITexture> texture_ptr);

    [[nodiscard]] const Settings& GetQuadSettings() const noexcept     { return m_settings; }
    [[nodiscard]] const Rhi::ITexture&  GetTexture() const;

    virtual void Draw(Rhi::IRenderCommandList& cmd_list, Rhi::ICommandListDebugGroup* debug_group_ptr = nullptr) const;

protected:
    Rhi::IRenderPattern& GetRenderPattern() const noexcept { return *m_render_pattern_ptr; }
    const Rhi::IRenderContext& GetRenderContext() const noexcept;

private:
    void UpdateConstantsBuffer() const;

    [[nodiscard]] static Rhi::IShader::MacroDefinitions GetPixelShaderMacroDefinitions(TextureMode texture_mode);

    Settings                       m_settings;
    const Ptr<Rhi::ICommandQueue>  m_render_cmd_queue_ptr;
    const Ptr<Rhi::IRenderPattern> m_render_pattern_ptr;
    Ptr<Rhi::IRenderState>         m_render_state_ptr;
    Ptr<Rhi::IViewState>           m_view_state_ptr;
    Ptr<Rhi::IBufferSet>           m_vertex_buffer_set_ptr;
    Ptr<Rhi::IBuffer>              m_index_buffer_ptr;
    Ptr<Rhi::IBuffer>              m_const_buffer_ptr;
    Ptr<Rhi::ITexture>             m_texture_ptr;
    Ptr<Rhi::ISampler>             m_texture_sampler_ptr;
    Ptr<Rhi::IProgramBindings>     m_const_program_bindings_ptr;
};

} // namespace Methane::Graphics
