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

FILE: Methane/Graphics/ScreenQuad.cpp
Screen Quad rendering primitive.

******************************************************************************/

#include <Methane/Graphics/ScreenQuad.h>

#include <Methane/Graphics/Mesh/QuadMesh.hpp>
#include <Methane/Graphics/RenderCommandList.h>
#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

struct SHADER_STRUCT_ALIGN ScreenQuadConstants
{
    SHADER_FIELD_ALIGN Color4f blend_color;
};

struct ScreenQuadVertex
{
    Mesh::Position position;
    Mesh::TexCoord texcoord;

    inline static const Mesh::VertexLayout layout = {
        Mesh::VertexField::Position,
        Mesh::VertexField::TexCoord,
    };
};

ScreenQuad::ScreenQuad(RenderContext& context, Ptr<Texture> sp_texture, Settings settings)
    : m_settings(std::move(settings))
    , m_debug_region_name(m_settings.name + " Screen-Quad rendering")
    , m_sp_texture(std::move(sp_texture))
{
    ITT_FUNCTION_TASK();

    if (!m_sp_texture)
        throw std::invalid_argument("Screen-quad texture can not be empty.");

    QuadMesh<ScreenQuadVertex> quad_mesh(ScreenQuadVertex::layout, 2.f, 2.f);

    const RenderContext::Settings& context_settings = context.GetSettings();

    RenderState::Settings state_settings;
    state_settings.sp_program = Program::Create(context,
        Program::Settings
        {
            Program::Shaders
            {
                Shader::CreateVertex(context, { Data::ShaderProvider::Get(), { "ScreenQuad", "ScreenQuadVS" }, { } }),
                Shader::CreatePixel( context, { Data::ShaderProvider::Get(), { "ScreenQuad", "ScreenQuadPS" }, { } }),
            },
            Program::InputBufferLayouts
            {
                Program::InputBufferLayout
                {
                    Program::InputBufferLayout::ArgumentSemantics { quad_mesh.GetVertexLayout().GetSemantics() }
                }
            },
            Program::ArgumentDescriptions
            {
                { { Shader::Type::Pixel, "g_constants" }, Program::Argument::Modifiers::Constant },
                { { Shader::Type::Pixel, "g_texture"   }, Program::Argument::Modifiers::Constant },
                { { Shader::Type::Pixel, "g_sampler"   }, Program::Argument::Modifiers::Constant },
            },
            PixelFormats
            {
                context_settings.color_format
            },
            context_settings.depth_stencil_format
        }
    );
    state_settings.sp_program->SetName(m_settings.name + " Screen-Quad Shading");
    state_settings.viewports            = { GetFrameViewport(settings.screen_rect) };
    state_settings.scissor_rects        = { GetFrameScissorRect(settings.screen_rect) };
    state_settings.depth.enabled        = false;
    state_settings.depth.write_enabled  = false;
    state_settings.rasterizer.is_front_counter_clockwise = true;
    state_settings.blending.render_targets[0].blend_enabled             = m_settings.alpha_blending_enabled;
    state_settings.blending.render_targets[0].source_rgb_blend_factor   = RenderState::Blending::Factor::SourceAlpha;
    state_settings.blending.render_targets[0].dest_rgb_blend_factor     = RenderState::Blending::Factor::OneMinusSourceAlpha;
    state_settings.blending.render_targets[0].source_alpha_blend_factor = RenderState::Blending::Factor::Zero;
    state_settings.blending.render_targets[0].dest_alpha_blend_factor   = RenderState::Blending::Factor::Zero;

    m_sp_state = RenderState::Create(context, state_settings);
    m_sp_state->SetName(m_settings.name + " Screen-Quad Render State");

    m_sp_texture_sampler = Sampler::Create(context, {
        { Sampler::Filter::MinMag::Linear     },
        { Sampler::Address::Mode::ClampToZero },
    });
    m_sp_texture_sampler->SetName(m_settings.name + " Screen-Quad Texture Sampler");
    m_sp_texture->SetName(m_settings.name + " Screen-Quad Texture");

    m_sp_vertex_buffer = Buffer::CreateVertexBuffer(context, static_cast<Data::Size>(quad_mesh.GetVertexDataSize()),
                                                             static_cast<Data::Size>(quad_mesh.GetVertexSize()));
    m_sp_vertex_buffer->SetName(m_settings.name + " Screen-Quad Vertex Buffer");
    m_sp_vertex_buffer->SetData({
        {
            reinterpret_cast<Data::ConstRawPtr>(quad_mesh.GetVertices().data()),
            static_cast<Data::Size>(quad_mesh.GetVertexDataSize())
        }
    });

    m_sp_index_buffer = Buffer::CreateIndexBuffer(context, static_cast<Data::Size>(quad_mesh.GetIndexDataSize()),
                                                           GetIndexFormat(quad_mesh.GetIndex(0)));
    m_sp_index_buffer->SetName(m_settings.name + " Screen-Quad Index Buffer");
    m_sp_index_buffer->SetData({
        {
            reinterpret_cast<Data::ConstRawPtr>(quad_mesh.GetIndices().data()),
            static_cast<Data::Size>(quad_mesh.GetIndexDataSize())
        }
    });

    const Data::Size const_buffer_size = static_cast<Data::Size>(sizeof(ScreenQuadConstants));
    m_sp_const_buffer = Buffer::CreateConstantBuffer(context, Buffer::GetAlignedBufferSize(const_buffer_size));
    m_sp_const_buffer->SetName(m_settings.name + " Screen-Quad Constants Buffer");

    m_sp_const_program_bindings = ProgramBindings::Create(state_settings.sp_program, {
        { { Shader::Type::Pixel, "g_constants" }, { { m_sp_const_buffer    } } },
        { { Shader::Type::Pixel, "g_texture"   }, { { m_sp_texture         } } },
        { { Shader::Type::Pixel, "g_sampler"   }, { { m_sp_texture_sampler } } },
    });

    UpdateConstantsBuffer();
}

void ScreenQuad::SetBlendColor(const Color4f& blend_color)
{
    ITT_FUNCTION_TASK();

    if (m_settings.blend_color == blend_color)
        return;

    m_settings.blend_color  = blend_color;

    UpdateConstantsBuffer();
}

void ScreenQuad::SetScreenRect(const FrameRect& screen_rect)
{
    ITT_FUNCTION_TASK();

    if (m_settings.screen_rect == screen_rect)
        return;

    m_settings.screen_rect = screen_rect;

    m_sp_state->SetViewports({ GetFrameViewport(screen_rect) });
    m_sp_state->SetScissorRects({ GetFrameScissorRect(screen_rect) });
}

void ScreenQuad::SetAlphaBlendingEnabled(bool alpha_blending_enabled)
{
    ITT_FUNCTION_TASK();

    if (m_settings.alpha_blending_enabled == alpha_blending_enabled)
        return;

    m_settings.alpha_blending_enabled = alpha_blending_enabled;

    RenderState::Settings state_settings = m_sp_state->GetSettings();
    state_settings.blending.render_targets[0].blend_enabled = alpha_blending_enabled;
    m_sp_state->Reset(state_settings);
}

void ScreenQuad::Draw(RenderCommandList& cmd_list) const
{
    ITT_FUNCTION_TASK();
    
    cmd_list.Reset(m_sp_state, m_debug_region_name);
    cmd_list.SetProgramBindings(*m_sp_const_program_bindings);
    cmd_list.SetVertexBuffers({ *m_sp_vertex_buffer });
    cmd_list.DrawIndexed(RenderCommandList::Primitive::Triangle, *m_sp_index_buffer);
}

void ScreenQuad::UpdateConstantsBuffer() const
{
    ScreenQuadConstants constants = {
        m_settings.blend_color
    };

    m_sp_const_buffer->SetData({
        {
            reinterpret_cast<Data::ConstRawPtr>(&constants),
            static_cast<Data::Size>(sizeof(constants))
        }
    });
}

} // namespace Methane::Graphics
