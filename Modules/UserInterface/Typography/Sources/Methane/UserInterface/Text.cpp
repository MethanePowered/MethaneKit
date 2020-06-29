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

FILE: Methane/UserInterface/Text.h
Methane text rendering primitive.

******************************************************************************/

#include "TextMesh.h"

#include <Methane/UserInterface/Text.h>

#include <Methane/Graphics/RenderContext.h>
#include <Methane/Graphics/RenderCommandList.h>
#include <Methane/Graphics/Texture.h>
#include <Methane/Graphics/Buffer.h>
#include <Methane/Graphics/RenderState.h>
#include <Methane/Graphics/Program.h>
#include <Methane/Graphics/ProgramBindings.h>
#include <Methane/Graphics/Sampler.h>
#include <Methane/Graphics/Types.h>
#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Instrumentation.h>

namespace Methane::UserInterface
{

struct SHADER_STRUCT_ALIGN Text::Constants
{
    SHADER_FIELD_ALIGN gfx::Color4f color;
};

Text::Text(gfx::RenderContext& context, Font& font, const SettingsUtf8&  settings)
    : Text(context, font,
        SettingsUtf32
        {
            settings.name,
            Font::ConvertUtf8To32(settings.text),
            settings.screen_rect,
            settings.screen_rect_in_pixels,
            settings.color,
            settings.wrap
        }
    )
{
    META_FUNCTION_TASK();
}

Text::Text(gfx::RenderContext& context, Font& font, SettingsUtf32 settings)
    : m_settings(std::move(settings))
    , m_context(context)
    , m_sp_font(font.shared_from_this())
    , m_sp_atlas_texture(font.GetAtlasTexturePtr(context))
{
    META_FUNCTION_TASK();

    m_sp_font->Connect(*this);

    const gfx::RenderContext::Settings& context_settings = context.GetSettings();

    m_viewport_rect = m_settings.screen_rect;
    if (!m_settings.screen_rect_in_pixels)
        m_viewport_rect *= m_context.GetContentScalingFactor();

    UpdateMeshData();
    UpdateMeshBuffers();

    m_sp_new_const_data = std::make_unique<Constants>(Constants{ m_settings.color });
    UpdateConstantsBuffer();

    gfx::RenderState::Settings state_settings;
    state_settings.sp_program = gfx::Program::Create(context,
        gfx::Program::Settings
        {
            gfx::Program::Shaders
            {
                gfx::Shader::CreateVertex(context, { Data::ShaderProvider::Get(), { "Text", "TextVS" }, { } }),
                gfx::Shader::CreatePixel( context, { Data::ShaderProvider::Get(), { "Text", "TextPS" }, { } }),
            },
            gfx::Program::InputBufferLayouts
            {
                gfx::Program::InputBufferLayout
                {
                    gfx::Program::InputBufferLayout::ArgumentSemantics { "POSITION", "TEXCOORD" }
                }
            },
            gfx::Program::ArgumentDescriptions
            {
                { { gfx::Shader::Type::Pixel, "g_constants" }, gfx::Program::Argument::Modifiers::Constant },
                { { gfx::Shader::Type::Pixel, "g_texture"   }, gfx::Program::Argument::Modifiers::None     },
                { { gfx::Shader::Type::Pixel, "g_sampler"   }, gfx::Program::Argument::Modifiers::Constant },
            },
            gfx::PixelFormats
            {
                context_settings.color_format
            },
            context_settings.depth_stencil_format
        }
    );
    state_settings.sp_program->SetName(m_settings.name + " Screen-Quad Shading");
    state_settings.viewports                                            = { gfx::GetFrameViewport(m_viewport_rect) };
    state_settings.scissor_rects                                        = { gfx::GetFrameScissorRect(m_viewport_rect) };
    state_settings.depth.enabled                                        = false;
    state_settings.depth.write_enabled                                  = false;
    state_settings.rasterizer.is_front_counter_clockwise                = true;
    state_settings.blending.render_targets[0].blend_enabled             = true;
    state_settings.blending.render_targets[0].source_rgb_blend_factor   = gfx::RenderState::Blending::Factor::SourceAlpha;
    state_settings.blending.render_targets[0].dest_rgb_blend_factor     = gfx::RenderState::Blending::Factor::OneMinusSourceAlpha;
    state_settings.blending.render_targets[0].source_alpha_blend_factor = gfx::RenderState::Blending::Factor::Zero;
    state_settings.blending.render_targets[0].dest_alpha_blend_factor   = gfx::RenderState::Blending::Factor::Zero;

    m_sp_state = gfx::RenderState::Create(context, state_settings);
    m_sp_state->SetName(m_settings.name + " Screen-Quad Render State");

    m_sp_texture_sampler = gfx::Sampler::Create(context, {
        { gfx::Sampler::Filter::MinMag::Linear     },
        { gfx::Sampler::Address::Mode::ClampToZero },
    });
    m_sp_texture_sampler->SetName(m_settings.name + " Screen-Quad Texture Sampler");

    m_sp_const_program_bindings = CreateConstProgramBindings();
}

Text::~Text() = default;

std::string Text::GetTextUtf8() const
{
    META_FUNCTION_TASK();
    return Font::ConvertUtf32To8(m_settings.text);
}

gfx::FrameRect Text::GetViewportInDots() const noexcept
{
    return m_viewport_rect / m_context.GetContentScalingFactor();
}

void Text::SetText(const std::string& text)
{
    META_FUNCTION_TASK();
    SetTextInScreenRect(text, m_settings.screen_rect, m_settings.screen_rect_in_pixels);
}

void Text::SetText(const std::u32string& text)
{
    META_FUNCTION_TASK();
    SetTextInScreenRect(text, m_settings.screen_rect, m_settings.screen_rect_in_pixels);
}

void Text::SetTextInScreenRect(const std::string& text, const gfx::FrameRect& screen_rect, bool rect_in_pixels)
{
    META_FUNCTION_TASK();
    SetTextInScreenRect(Font::ConvertUtf8To32(text), screen_rect, rect_in_pixels);
}

void Text::SetTextInScreenRect(const std::u32string& text, const gfx::FrameRect& screen_rect, bool rect_in_pixels)
{
    META_FUNCTION_TASK();
    if (m_settings.text == text && m_settings.screen_rect == screen_rect && m_settings.screen_rect_in_pixels == rect_in_pixels)
        return;

    m_settings.text = text;
    m_settings.screen_rect = screen_rect;
    m_settings.screen_rect_in_pixels = rect_in_pixels;

    if (m_settings.text.empty())
    {
        m_sp_new_mesh_data.reset();
        return;
    }

    m_viewport_rect = m_settings.screen_rect;
    if (!m_settings.screen_rect_in_pixels)
        m_viewport_rect *= m_context.GetContentScalingFactor();

    if (!m_sp_atlas_texture && !m_settings.text.empty())
    {
        m_sp_font->AddChars(Font::GetAlphabetFromText(m_settings.text));
        m_sp_new_atlas_texture = m_sp_font->GetAtlasTexturePtr(m_context);
    }

    UpdateMeshData();

    m_sp_state->SetViewports({ gfx::GetFrameViewport(m_viewport_rect) });
    m_sp_state->SetScissorRects({ gfx::GetFrameScissorRect(m_viewport_rect) });
}

void Text::SetScreenRect(const gfx::FrameRect& screen_rect, bool rect_in_pixels)
{
    META_FUNCTION_TASK();
    if (m_settings.screen_rect == screen_rect && m_settings.screen_rect_in_pixels == rect_in_pixels)
        return;

    m_settings.screen_rect = screen_rect;
    m_settings.screen_rect_in_pixels = rect_in_pixels;

    m_viewport_rect = screen_rect;
    if (!rect_in_pixels)
        m_viewport_rect *= m_context.GetContentScalingFactor();

    UpdateMeshData();

    m_sp_state->SetViewports({ gfx::GetFrameViewport(m_viewport_rect) });
    m_sp_state->SetScissorRects({ gfx::GetFrameScissorRect(m_viewport_rect) });
}

void Text::SetColor(const gfx::Color4f& color)
{
    META_FUNCTION_TASK();
    if (m_settings.color == color)
        return;

    m_settings.color = color;
    m_sp_new_const_data = std::make_unique<Constants>(Constants{
        m_settings.color
    });
}

void Text::Draw(gfx::RenderCommandList& cmd_list)
{
    META_FUNCTION_TASK();
    if (m_settings.text.empty() || (!m_sp_new_atlas_texture && !m_sp_atlas_texture))
        return;

    UpdateAtlasTexture();
    UpdateMeshBuffers();
    UpdateConstantsBuffer();

    assert(m_sp_const_program_bindings);
    assert(m_sp_vertex_buffers);
    assert(m_sp_index_buffer);

    cmd_list.Reset(m_sp_state);
    cmd_list.SetProgramBindings(*m_sp_const_program_bindings);
    cmd_list.SetVertexBuffers(*m_sp_vertex_buffers);
    cmd_list.DrawIndexed(gfx::RenderCommandList::Primitive::Triangle, *m_sp_index_buffer);
}

void Text::OnFontAtlasTextureReset(Font& font, const Ptr<gfx::Texture>& sp_old_atlas_texture, const Ptr<gfx::Texture>& sp_new_atlas_texture)
{
    META_FUNCTION_TASK();
    META_UNUSED(sp_old_atlas_texture);

    if (!sp_new_atlas_texture)
    {
        assert(m_sp_atlas_texture.get() == sp_old_atlas_texture.get());
        m_sp_atlas_texture.reset();
        m_sp_new_atlas_texture.reset();
        m_sp_const_program_bindings.reset();
        return;
    }

    if (m_sp_font.get() != std::addressof(font) ||
        m_sp_new_atlas_texture.get() == sp_new_atlas_texture.get() ||
        std::addressof(m_context) != std::addressof(sp_new_atlas_texture->GetContext()))
        return;

    assert(m_sp_atlas_texture.get() == sp_old_atlas_texture.get());
    m_sp_new_atlas_texture = sp_new_atlas_texture;
}

Ptr<gfx::ProgramBindings> Text::CreateConstProgramBindings()
{
    META_FUNCTION_TASK();
    if (!m_sp_const_buffer || !m_sp_atlas_texture || !m_sp_texture_sampler)
        return nullptr;

    return gfx::ProgramBindings::Create(m_sp_state->GetSettings().sp_program, {
        { { gfx::Shader::Type::Pixel, "g_constants" }, { { m_sp_const_buffer    } } },
        { { gfx::Shader::Type::Pixel, "g_texture"   }, { { m_sp_atlas_texture   } } },
        { { gfx::Shader::Type::Pixel, "g_sampler"   }, { { m_sp_texture_sampler } } },
    });
}

void Text::UpdateAtlasTexture()
{
    META_FUNCTION_TASK();
    if (!m_sp_new_atlas_texture)
        return;

    m_sp_atlas_texture = m_sp_new_atlas_texture;
    m_sp_new_atlas_texture.reset();

    if (m_sp_const_program_bindings)
    {
        const Ptr<gfx::ProgramBindings::ArgumentBinding>& sp_atlas_texture_binding = m_sp_const_program_bindings->Get({ gfx::Shader::Type::Pixel, "g_texture" });
        if (!sp_atlas_texture_binding)
            throw std::logic_error("Can not find atlas texture argument binding.");

        sp_atlas_texture_binding->SetResourceLocations({ { m_sp_atlas_texture } });
    }
    else
    {
        m_sp_const_program_bindings = CreateConstProgramBindings();
    }
}

void Text::UpdateMeshData()
{
    META_FUNCTION_TASK();
    if (m_sp_font->GetAtlasSize())
    {
        m_sp_new_mesh_data = std::make_unique<TextMesh>(m_settings.text, m_settings.wrap, *m_sp_font, m_viewport_rect.size);
    }
}

void Text::UpdateMeshBuffers()
{
    META_FUNCTION_TASK();
    if (!m_sp_new_mesh_data)
        return;

    // Update vertex buffer
    const Data::Size vertices_data_size = m_sp_new_mesh_data->GetVerticesDataSize();
    if (!m_sp_vertex_buffers || (*m_sp_vertex_buffers)[0].GetDataSize() < vertices_data_size)
    {
        Ptr<gfx::Buffer> sp_vertex_buffer = gfx::Buffer::CreateVertexBuffer(m_context, vertices_data_size, m_sp_new_mesh_data->GetVertexSize());
        sp_vertex_buffer->SetName(m_settings.name + " Text Vertex Buffer");
        m_sp_vertex_buffers = gfx::BufferSet::CreateVertexBuffers({ *sp_vertex_buffer });
    }
    (*m_sp_vertex_buffers)[0].SetData({
        gfx::Resource::SubResource(
            reinterpret_cast<Data::ConstRawPtr>(m_sp_new_mesh_data->GetVertices().data()), vertices_data_size,
            gfx::Resource::SubResource::Index(), gfx::Resource::BytesRange(0u, vertices_data_size)
        )
    });

    // Update index buffer
    const Data::Size indices_data_size = m_sp_new_mesh_data->GetIndicesDataSize();
    if (!m_sp_index_buffer || m_sp_index_buffer->GetDataSize() < indices_data_size)
    {
        m_sp_index_buffer = gfx::Buffer::CreateIndexBuffer(m_context, indices_data_size, gfx::PixelFormat::R16Uint);
        m_sp_index_buffer->SetName(m_settings.name + " Text Index Buffer");
    }
    m_sp_index_buffer->SetData({
        gfx::Resource::SubResource(
            reinterpret_cast<Data::ConstRawPtr>(m_sp_new_mesh_data->GetIndices().data()), indices_data_size,
            gfx::Resource::SubResource::Index(), gfx::Resource::BytesRange(0u, indices_data_size)
        )
    });

    m_sp_new_mesh_data.reset();
}

void Text::UpdateConstantsBuffer()
{
    META_FUNCTION_TASK();
    if (!m_sp_new_const_data)
        return;

    const Data::Size const_data_size = static_cast<Data::Size>(sizeof(Constants));
    if (!m_sp_const_buffer)
    {
        m_sp_const_buffer = gfx::Buffer::CreateConstantBuffer(m_context, gfx::Buffer::GetAlignedBufferSize(const_data_size));
        m_sp_const_buffer->SetName(m_settings.name + " Screen-Quad Constants Buffer");
    }
    m_sp_const_buffer->SetData({ { reinterpret_cast<Data::ConstRawPtr>(m_sp_new_const_data.get()), const_data_size } });

    m_sp_new_const_data.reset();
}

} // namespace Methane::Graphics
