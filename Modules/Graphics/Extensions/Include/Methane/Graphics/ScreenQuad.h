/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/ScreenQuad.h
ScreenQuad rendering primitive.

******************************************************************************/

#pragma once

#include <Methane/Graphics/RenderContext.h>
#include <Methane/Graphics/Texture.h>
#include <Methane/Graphics/Buffer.h>
#include <Methane/Graphics/RenderState.h>
#include <Methane/Graphics/Program.h>
#include <Methane/Graphics/ProgramBindings.h>
#include <Methane/Graphics/Sampler.h>
#include <Methane/Graphics/MathTypes.h>
#include <Methane/Graphics/Types.h>

#include <memory>

namespace Methane::Graphics
{

struct RenderCommandList;

class ScreenQuad
{
public:
    struct Settings
    {
        const std::string  name;
        FrameRect   screen_rect;
        bool        alpha_blending_enabled = false;
        Color4f     blend_color = Color4f(1.f, 1.f, 1.f, 1.f);
    };

    ScreenQuad(RenderContext& context, Ptr<Texture> sp_texture, Settings settings);

    void SetBlendColor(const Color4f& blend_color);
    void SetScreenRect(const FrameRect& screen_rect);
    void SetAlphaBlendingEnabled(bool alpha_blending_enabled);

    void Draw(RenderCommandList& cmd_list) const;

private:
    void UpdateConstantsBuffer() const;

    Settings             m_settings;
    const std::string    m_debug_region_name;
    Ptr<RenderState>     m_sp_state;
    Ptr<Buffer>          m_sp_vertex_buffer;
    Ptr<Buffer>          m_sp_index_buffer;
    Ptr<Buffer>          m_sp_const_buffer;
    Ptr<Texture>         m_sp_texture;
    Ptr<Sampler>         m_sp_texture_sampler;
    Ptr<ProgramBindings> m_sp_const_program_bindings;
};

} // namespace Methane::Graphics
