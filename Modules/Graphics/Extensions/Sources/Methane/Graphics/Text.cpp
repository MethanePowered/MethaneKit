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

FILE: Methane/Graphics/Text.cpp
Screen Quad rendering primitive.

******************************************************************************/

#include <Methane/Graphics/Text.h>
#include <Methane/Graphics/Font.h>

#include <Methane/Graphics/Mesh/QuadMesh.hpp>
#include <Methane/Graphics/RenderCommandList.h>
#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

struct SHADER_STRUCT_ALIGN TextConstants
{
    SHADER_FIELD_ALIGN Color4f blend_color;
};

struct TextVertex
{
    Mesh::Position position;
    Mesh::TexCoord texcoord;

    inline static const Mesh::VertexLayout layout = {
        Mesh::VertexField::Position,
        Mesh::VertexField::TexCoord,
    };
};

Text::Text(RenderContext& context, Settings settings)
    : m_settings(std::move(settings))
    , m_debug_region_name(m_settings.name + " Text Render")
{
    ITT_FUNCTION_TASK();

    Font::Library& font_library = Font::Library::Get();

    QuadMesh<TextVertex> quad_mesh(TextVertex::layout, 2.f, 2.f);

    const RenderContext::Settings& context_settings = context.GetSettings();

    RenderState::Settings state_settings;
    state_settings.sp_program = Program::Create(context,
        Program::Settings
        {
            Program::Shaders
            {
                Shader::CreateVertex(context, { Data::ShaderProvider::Get(), { "Text", "TextVS" }, { } }),
                Shader::CreatePixel( context, { Data::ShaderProvider::Get(), { "Text", "TextPS" }, { } }),
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

    const Data::Size const_buffer_size = static_cast<Data::Size>(sizeof(TextConstants));
    m_sp_const_buffer = Buffer::CreateConstantBuffer(context, Buffer::GetAlignedBufferSize(const_buffer_size));
    m_sp_const_buffer->SetName(m_settings.name + " Screen-Quad Constants Buffer");

    /*
    m_sp_const_program_bindings = ProgramBindings::Create(state_settings.sp_program, {
        { { Shader::Type::Pixel, "g_constants" }, { { m_sp_const_buffer    } } },
        { { Shader::Type::Pixel, "g_texture"   }, { { m_sp_texture         } } },
        { { Shader::Type::Pixel, "g_sampler"   }, { { m_sp_texture_sampler } } },
    });
    */

    UpdateConstantsBuffer();
}

void Text::SetBlendColor(const Color4f& blend_color)
{
    ITT_FUNCTION_TASK();

    if (m_settings.blend_color == blend_color)
        return;

    m_settings.blend_color  = blend_color;

    UpdateConstantsBuffer();
}

void Text::SetScreenRect(const FrameRect& screen_rect)
{
    ITT_FUNCTION_TASK();

    if (m_settings.screen_rect == screen_rect)
        return;

    m_settings.screen_rect = screen_rect;

    m_sp_state->SetViewports({ GetFrameViewport(screen_rect) });
    m_sp_state->SetScissorRects({ GetFrameScissorRect(screen_rect) });
}

void Text::SetAlphaBlendingEnabled(bool alpha_blending_enabled)
{
    ITT_FUNCTION_TASK();

    if (m_settings.alpha_blending_enabled == alpha_blending_enabled)
        return;

    m_settings.alpha_blending_enabled = alpha_blending_enabled;

    RenderState::Settings state_settings = m_sp_state->GetSettings();
    state_settings.blending.render_targets[0].blend_enabled = alpha_blending_enabled;
    m_sp_state->Reset(state_settings);
}

void Text::Draw(RenderCommandList& cmd_list) const
{
    ITT_FUNCTION_TASK();
    
    cmd_list.Reset(m_sp_state, m_debug_region_name);
    cmd_list.SetProgramBindings(*m_sp_const_program_bindings);
    cmd_list.SetVertexBuffers({ *m_sp_vertex_buffer });
    cmd_list.DrawIndexed(RenderCommandList::Primitive::Triangle, *m_sp_index_buffer);
}

void Text::UpdateConstantsBuffer() const
{
    TextConstants constants = {
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
