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
#include <Methane/Graphics/TypeConverters.hpp>
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

    inline static const Mesh::VertexLayout layout {
        Mesh::VertexField::Position,
        Mesh::VertexField::TexCoord,
    };
};

ScreenQuad::ScreenQuad(RenderContext& context, Settings settings)
    : ScreenQuad(context, nullptr, settings)
{
}

ScreenQuad::ScreenQuad(RenderContext& context, Ptr<Texture> sp_texture, Settings settings)
    : m_settings(std::move(settings))
    , m_context(context)
    , m_sp_texture(std::move(sp_texture))
{
    META_FUNCTION_TASK();
    if (m_settings.texture_mode != TextureMode::Disabled && !m_sp_texture)
        throw std::invalid_argument("Screen-quad texture can not be empty when quad texturing is enabled.");

    QuadMesh<ScreenQuadVertex>     quad_mesh(ScreenQuadVertex::layout, 2.f, 2.f);
    const RenderContext::Settings& context_settings = context.GetSettings();
    const Shader::MacroDefinitions ps_macro_definitions = GetPixelShaderMacroDefinitions(m_settings.texture_mode, m_settings.texture_color_mode);
    Program::ArgumentDescriptions  program_argument_descriptions = {
        { { Shader::Type::Pixel, "g_constants" }, Program::Argument::Modifiers::Constant }
    };

    if (m_settings.texture_mode != TextureMode::Disabled)
    {
        const Program::Argument::Modifiers::Mask texture_argument_modifiers = m_settings.texture_mode == TextureMode::Constant
                                                                              ? Program::Argument::Modifiers::Constant
                                                                              : Program::Argument::Modifiers::None;
        program_argument_descriptions.emplace(Shader::Type::Pixel, "g_texture", texture_argument_modifiers);
        program_argument_descriptions.emplace(Shader::Type::Pixel, "g_sampler", Program::Argument::Modifiers::Constant);
    }

    RenderState::Settings state_settings;
    state_settings.sp_program = Program::Create(context,
        Program::Settings
        {
            Program::Shaders
            {
                Shader::CreateVertex(context, { Data::ShaderProvider::Get(), { "ScreenQuad", "QuadVS" }, { } }),
                Shader::CreatePixel( context, { Data::ShaderProvider::Get(), { "ScreenQuad", "QuadPS" }, ps_macro_definitions }),
            },
            Program::InputBufferLayouts
            {
                Program::InputBufferLayout
                {
                    Program::InputBufferLayout::ArgumentSemantics { quad_mesh.GetVertexLayout().GetSemantics() }
                }
            },
            program_argument_descriptions,
            PixelFormats
            {
                context_settings.color_format
            },
            context_settings.depth_stencil_format
        }
    );
    state_settings.sp_program->SetName(m_settings.name + " Screen-Quad Shading");
    state_settings.depth.enabled        = false;
    state_settings.depth.write_enabled  = false;
    state_settings.rasterizer.is_front_counter_clockwise = true;
    state_settings.blending.render_targets[0].blend_enabled             = m_settings.alpha_blending_enabled;
    state_settings.blending.render_targets[0].source_rgb_blend_factor   = RenderState::Blending::Factor::SourceAlpha;
    state_settings.blending.render_targets[0].dest_rgb_blend_factor     = RenderState::Blending::Factor::OneMinusSourceAlpha;
    state_settings.blending.render_targets[0].source_alpha_blend_factor = RenderState::Blending::Factor::Zero;
    state_settings.blending.render_targets[0].dest_alpha_blend_factor   = RenderState::Blending::Factor::Zero;

    m_sp_render_state = RenderState::Create(context, state_settings);
    m_sp_render_state->SetName(m_settings.name + " Screen-Quad Render State");

    m_sp_view_state = ViewState::Create({
        { GetFrameViewport(m_settings.screen_rect)    },
        { GetFrameScissorRect(m_settings.screen_rect) }
    });

    if (m_settings.texture_mode != TextureMode::Disabled)
    {
        m_sp_texture_sampler = Sampler::Create(context, {
            { Sampler::Filter::MinMag::Linear     },
            { Sampler::Address::Mode::ClampToZero },
            });
        m_sp_texture_sampler->SetName(m_settings.name + " Screen-Quad Texture Sampler");
        m_sp_texture->SetName(m_settings.name + " Screen-Quad Texture");
    }

    Ptr<Buffer> sp_vertex_buffer = Buffer::CreateVertexBuffer(context,
                                                              static_cast<Data::Size>(quad_mesh.GetVertexDataSize()),
                                                              static_cast<Data::Size>(quad_mesh.GetVertexSize()));
    sp_vertex_buffer->SetName(m_settings.name + " Screen-Quad Vertex Buffer");
    sp_vertex_buffer->SetData({
        {
            reinterpret_cast<Data::ConstRawPtr>(quad_mesh.GetVertices().data()),
            static_cast<Data::Size>(quad_mesh.GetVertexDataSize())
        }
    });
    m_sp_vertex_buffer_set = BufferSet::CreateVertexBuffers({ *sp_vertex_buffer });

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

    ProgramBindings::ResourceLocationsByArgument program_binding_resource_locations = {
        { { Shader::Type::Pixel, "g_constants" }, { { m_sp_const_buffer    } } }
    };

    if (m_settings.texture_mode != TextureMode::Disabled)
    {
        program_binding_resource_locations.emplace(Program::Argument(Shader::Type::Pixel, "g_texture"), Resource::Locations{ { m_sp_texture } });
        program_binding_resource_locations.emplace(Program::Argument(Shader::Type::Pixel, "g_sampler"), Resource::Locations{ { m_sp_texture_sampler } });
    }

    m_sp_const_program_bindings = ProgramBindings::Create(m_sp_render_state->GetSettings().sp_program, program_binding_resource_locations);

    UpdateConstantsBuffer();
}

void ScreenQuad::SetBlendColor(const Color4f& blend_color)
{
    META_FUNCTION_TASK();
    if (m_settings.blend_color == blend_color)
        return;

    m_settings.blend_color  = blend_color;

    UpdateConstantsBuffer();
}

void ScreenQuad::SetScreenRect(const FrameRect& screen_rect, const FrameSize& render_attachment_size)
{
    META_FUNCTION_TASK();
    if (m_settings.screen_rect == screen_rect)
        return;

    m_settings.screen_rect = screen_rect;

    m_sp_view_state->SetViewports({ GetFrameViewport(screen_rect) });
    m_sp_view_state->SetScissorRects({ GetFrameScissorRect(screen_rect, render_attachment_size) });
}

void ScreenQuad::SetAlphaBlendingEnabled(bool alpha_blending_enabled)
{
    META_FUNCTION_TASK();
    if (m_settings.alpha_blending_enabled == alpha_blending_enabled)
        return;

    m_settings.alpha_blending_enabled = alpha_blending_enabled;

    RenderState::Settings state_settings = m_sp_render_state->GetSettings();
    state_settings.blending.render_targets[0].blend_enabled = alpha_blending_enabled;
    m_sp_render_state->Reset(state_settings);
}

void ScreenQuad::SetTexture(Ptr<Texture> sp_texture)
{
    META_FUNCTION_TASK();
    if (m_settings.texture_mode != TextureMode::Volatile)
        throw std::logic_error("Can not change texture of screen quad when texture argument is not configured as volatile");

    if (m_sp_texture.get() == sp_texture.get())
        return;

    if (!sp_texture)
        throw std::invalid_argument("Can not set null texture to screen quad.");

    m_sp_texture = sp_texture;

    const Ptr<ProgramBindings::ArgumentBinding>& sp_texture_binding = m_sp_const_program_bindings->Get({ Shader::Type::Pixel, "g_texture" });
    if (!sp_texture_binding)
        throw std::logic_error("Can not find screen quad texture argument binding.");

    sp_texture_binding->SetResourceLocations({ { m_sp_texture } });
}

const Texture& ScreenQuad::GetTexture() const noexcept
{
    META_FUNCTION_TASK();
    assert(!!m_sp_texture);
    return *m_sp_texture;
}

void ScreenQuad::Draw(RenderCommandList& cmd_list, CommandList::DebugGroup* p_debug_group) const
{
    META_FUNCTION_TASK();
    cmd_list.Reset(m_sp_render_state, p_debug_group);
    cmd_list.SetViewState(*m_sp_view_state);
    cmd_list.SetProgramBindings(*m_sp_const_program_bindings);
    cmd_list.SetVertexBuffers(*m_sp_vertex_buffer_set);
    cmd_list.DrawIndexed(RenderCommandList::Primitive::Triangle, *m_sp_index_buffer);
}

void ScreenQuad::UpdateConstantsBuffer() const
{
    META_FUNCTION_TASK();
    const ScreenQuadConstants constants {
        m_settings.blend_color
    };

    m_sp_const_buffer->SetData({
        {
            reinterpret_cast<Data::ConstRawPtr>(&constants),
            static_cast<Data::Size>(sizeof(constants))
        }
    });
}

Shader::MacroDefinitions ScreenQuad::GetPixelShaderMacroDefinitions(TextureMode texture_mode, TextureColorMode color_mode)
{
    META_FUNCTION_TASK();
    Shader::MacroDefinitions macro_definitions;
    if (texture_mode == TextureMode::Disabled)
        macro_definitions.emplace_back("TEXTURE_DISABLED", "");

    switch(color_mode)
    {
    case TextureColorMode::RgbaFloat:
        break;

    case TextureColorMode::RFloatToAlpha:
        macro_definitions.emplace_back("TTEXEL", "float");
        macro_definitions.emplace_back("RMASK", "r");
        macro_definitions.emplace_back("WMASK", "a");
        break;
    };

    return macro_definitions;
}

} // namespace Methane::Graphics
