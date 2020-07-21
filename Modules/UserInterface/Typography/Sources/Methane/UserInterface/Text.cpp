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
#include <Methane/UserInterface/Context.h>

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

#include <cml/mathlib/mathlib.h>

namespace Methane::UserInterface
{

struct SHADER_STRUCT_ALIGN Text::Constants
{
    SHADER_FIELD_ALIGN gfx::Color4f color;
};

struct SHADER_STRUCT_ALIGN Text::Uniforms
{
    SHADER_FIELD_ALIGN gfx::Matrix44f vp_matrix;
};

Text::Text(Context& ui_context, Font& font, const SettingsUtf8&  settings)
    : Text(ui_context, font,
        SettingsUtf32
        {
            settings.name,
            Font::ConvertUtf8To32(settings.text),
            settings.rect,
            settings.color,
            settings.wrap
        }
    )
{
    META_FUNCTION_TASK();
}

Text::Text(Context& ui_context, Font& font, SettingsUtf32 settings)
    : Item(ui_context, settings.rect)
    , m_settings(std::move(settings))
    , m_sp_font(font.shared_from_this())
    , m_sp_atlas_texture(font.GetAtlasTexturePtr(GetUIContext().GetRenderContext()))
{
    META_FUNCTION_TASK();
    m_sp_font->Connect(*this);

    const gfx::RenderContext::Settings& context_settings = GetUIContext().GetRenderContext().GetSettings();
    m_viewport_rect = GetUIContext().ConvertToPixels(m_settings.rect);

    UpdateMeshData();
    UpdateUniformsBuffer();
    UpdateConstantsBuffer();

    gfx::RenderState::Settings state_settings;
    state_settings.sp_program = gfx::Program::Create(GetUIContext().GetRenderContext(),
        gfx::Program::Settings
        {
            gfx::Program::Shaders
            {
                gfx::Shader::CreateVertex(GetUIContext().GetRenderContext(), { Data::ShaderProvider::Get(), { "Text", "TextVS" }, { } }),
                gfx::Shader::CreatePixel( GetUIContext().GetRenderContext(), { Data::ShaderProvider::Get(), { "Text", "TextPS" }, { } }),
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
                { { gfx::Shader::Type::Vertex, "g_uniforms"  }, gfx::Program::Argument::Modifiers::Constant },
                { { gfx::Shader::Type::Pixel,  "g_constants" }, gfx::Program::Argument::Modifiers::Constant },
                { { gfx::Shader::Type::Pixel,  "g_texture"   }, gfx::Program::Argument::Modifiers::None     },
                { { gfx::Shader::Type::Pixel,  "g_sampler"   }, gfx::Program::Argument::Modifiers::Constant },
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

    m_sp_state = gfx::RenderState::Create(GetUIContext().GetRenderContext(), state_settings);
    m_sp_state->SetName(m_settings.name + " Screen-Quad Render State");

    m_sp_texture_sampler = gfx::Sampler::Create(GetUIContext().GetRenderContext(), {
        { gfx::Sampler::Filter::MinMag::Linear     },
        { gfx::Sampler::Address::Mode::ClampToZero },
    });
    m_sp_texture_sampler->SetName(m_settings.name + " Screen-Quad Texture Sampler");

    m_sp_curr_program_bindings = CreateProgramBindings();
}

Text::~Text()
{
    META_FUNCTION_TASK();

    // Manually disconnect font, so that if it will be released along with text,
    // the destroyed text won't receive font atlas update callback leading to access violation
    m_sp_font->Disconnect(*this);
}

std::string Text::GetTextUtf8() const
{
    META_FUNCTION_TASK();
    return Font::ConvertUtf32To8(m_settings.text);
}

UnitRect Text::GetViewportInDots() const
{
    META_FUNCTION_TASK();
    return GetUIContext().ConvertToDots(m_viewport_rect);
}

void Text::SetText(const std::string& text)
{
    META_FUNCTION_TASK();
    SetTextInScreenRect(text, m_settings.rect);
}

void Text::SetText(const std::u32string& text)
{
    META_FUNCTION_TASK();
    SetTextInScreenRect(text, m_settings.rect);
}

void Text::SetTextInScreenRect(const std::string& text, const UnitRect& ui_rect)
{
    META_FUNCTION_TASK();
    SetTextInScreenRect(Font::ConvertUtf8To32(text), ui_rect);
}

void Text::SetTextInScreenRect(const std::u32string& text, const UnitRect& ui_rect)
{
    META_FUNCTION_TASK();
    const UnitRect ui_rect_in_units = GetUIContext().ConvertToUnits(ui_rect, m_settings.rect.units);
    if (m_settings.text == text && m_settings.rect == ui_rect_in_units)
        return;

    const bool text_changed             = m_settings.text != text;
    const bool screen_rect_size_changed = m_settings.rect.size != ui_rect_in_units.size;

    m_settings.text = text;
    m_settings.rect = ui_rect_in_units;
    m_viewport_rect = GetUIContext().ConvertToPixels(m_settings.rect);

    if (screen_rect_size_changed || text_changed)
    {
        UpdateMeshData();
        UpdateUniformsBuffer();
    }

    if (!m_sp_atlas_texture)
    {
        // If atlas texture was not initialized it has to be requested for current context first to be properly updated in future
        UpdateAtlasTexture(m_sp_font->GetAtlasTexturePtr(GetUIContext().GetRenderContext()));
    }

    m_sp_state->SetViewports({ gfx::GetFrameViewport(m_viewport_rect) });
    m_sp_state->SetScissorRects({ gfx::GetFrameScissorRect(m_viewport_rect) });
}

bool Text::SetRect(const UnitRect& ui_rect)
{
    META_FUNCTION_TASK();
    const UnitRect ui_rect_in_units = GetUIContext().ConvertToUnits(ui_rect, m_settings.rect.units);
    const bool screen_rect_size_changed = m_settings.rect.size != ui_rect_in_units.size;

    m_settings.rect = ui_rect_in_units;
    if (screen_rect_size_changed)
        m_viewport_rect = GetUIContext().ConvertToPixels(m_settings.rect);
    else
        m_viewport_rect.origin = GetUIContext().ConvertToPixels(m_settings.rect).origin;

    if (screen_rect_size_changed)
    {
        UpdateMeshData();
        UpdateUniformsBuffer();
    }

    m_sp_state->SetViewports({ gfx::GetFrameViewport(m_viewport_rect) });
    m_sp_state->SetScissorRects({ gfx::GetFrameScissorRect(m_viewport_rect) });
    return Item::SetRect(GetUIContext().ConvertToUnits(m_viewport_rect, ui_rect.units));
}

void Text::SetColor(const gfx::Color4f& color)
{
    META_FUNCTION_TASK();
    if (m_settings.color == color)
        return;

    m_settings.color = color;
    UpdateConstantsBuffer();
}

void Text::Draw(gfx::RenderCommandList& cmd_list)
{
    META_FUNCTION_TASK();
    if (m_settings.text.empty() || !m_sp_atlas_texture)
        return;

    assert(m_sp_curr_program_bindings);
    assert(m_sp_vertex_buffer_set);
    assert(m_sp_index_buffer);

    cmd_list.Reset(m_sp_state);
    cmd_list.SetProgramBindings(*m_sp_curr_program_bindings);
    cmd_list.SetVertexBuffers(*m_sp_vertex_buffer_set);
    cmd_list.DrawIndexed(gfx::RenderCommandList::Primitive::Triangle, *m_sp_index_buffer);

    if (m_sp_prev_program_bindings && m_prev_program_bindings_release_on_frame == GetUIContext().GetRenderContext().GetFrameBufferIndex())
        m_sp_prev_program_bindings.reset();
}

void Text::OnFontAtlasTextureReset(Font& font, const Ptr<gfx::Texture>& sp_old_atlas_texture, const Ptr<gfx::Texture>& sp_new_atlas_texture)
{
    META_FUNCTION_TASK();
    META_UNUSED(sp_old_atlas_texture);
    if (m_sp_font.get() != std::addressof(font) || (sp_new_atlas_texture && std::addressof(GetUIContext().GetRenderContext()) != std::addressof(sp_new_atlas_texture->GetContext())))
        return;

    assert(m_sp_atlas_texture.get() == sp_old_atlas_texture.get());
    UpdateAtlasTexture(sp_new_atlas_texture);

    if (m_sp_text_mesh)
    {
        // Reset text mesh along with font atlas for texture coordinates to match atlas
        m_sp_text_mesh.reset();
        UpdateMeshData();
    }
}

Ptr<gfx::ProgramBindings> Text::CreateProgramBindings()
{
    META_FUNCTION_TASK();
    if (!m_sp_uniforms_buffer || !m_sp_const_buffer || !m_sp_atlas_texture || !m_sp_texture_sampler)
        return nullptr;

    return gfx::ProgramBindings::Create(m_sp_state->GetSettings().sp_program, {
        { { gfx::Shader::Type::Vertex, "g_uniforms"  }, { { m_sp_uniforms_buffer } } },
        { { gfx::Shader::Type::Pixel,  "g_constants" }, { { m_sp_const_buffer    } } },
        { { gfx::Shader::Type::Pixel,  "g_texture"   }, { { m_sp_atlas_texture   } } },
        { { gfx::Shader::Type::Pixel,  "g_sampler"   }, { { m_sp_texture_sampler } } },
    });
}

void Text::UpdateAtlasTexture(const Ptr<gfx::Texture>& sp_new_atlas_texture)
{
    META_FUNCTION_TASK();
    if (m_sp_atlas_texture.get() == sp_new_atlas_texture.get())
        return;

    m_sp_atlas_texture = sp_new_atlas_texture;

    // Retain previous program bindings until they are used by in flight command lists
    m_sp_prev_program_bindings = m_sp_curr_program_bindings;
    m_prev_program_bindings_release_on_frame = GetUIContext().GetRenderContext().GetFrameBufferIndex();

    if (!m_sp_atlas_texture)
    {
        m_sp_curr_program_bindings.reset();
        return;
    }

    if (m_sp_prev_program_bindings)
    {
        m_sp_curr_program_bindings = gfx::ProgramBindings::CreateCopy(*m_sp_prev_program_bindings, {
            { { gfx::Shader::Type::Pixel, "g_texture" }, { { m_sp_atlas_texture } } },
        });
    }
    else
    {
        m_sp_curr_program_bindings = CreateProgramBindings();
    }
}

void Text::UpdateMeshData()
{
    META_FUNCTION_TASK();
    if (m_settings.text.empty())
        return;

    // Fill font with new text chars strictly before building the text mesh, to be sure that font atlas size is up to date
    m_sp_font->AddChars(m_settings.text);

    if (!m_sp_font->GetAtlasSize())
        return;

    if (m_settings.incremental_update && m_sp_text_mesh &&
        m_sp_text_mesh->IsUpdatable(m_settings.text, m_settings.wrap, *m_sp_font, m_viewport_rect.size))
    {
        m_sp_text_mesh->Update(m_settings.text, m_viewport_rect.size);
    }
    else
    {
        m_sp_text_mesh = std::make_unique<TextMesh>(m_settings.text, m_settings.wrap, *m_sp_font, m_viewport_rect.size);
    }

    // Update vertex buffer
    const Data::Size vertices_data_size = m_sp_text_mesh->GetVerticesDataSize();
    assert(vertices_data_size);
    if (!vertices_data_size)
        return;

    if (!m_sp_vertex_buffer_set || (*m_sp_vertex_buffer_set)[0].GetDataSize() < vertices_data_size)
    {
        const Data::Size vertex_buffer_size = vertices_data_size * m_settings.mesh_buffers_reservation_multiplier;
        Ptr<gfx::Buffer> sp_vertex_buffer = gfx::Buffer::CreateVertexBuffer(GetUIContext().GetRenderContext(), vertex_buffer_size, m_sp_text_mesh->GetVertexSize());
        sp_vertex_buffer->SetName(m_settings.name + " Text Vertex Buffer");
        m_sp_vertex_buffer_set = gfx::BufferSet::CreateVertexBuffers({ *sp_vertex_buffer });
    }
    (*m_sp_vertex_buffer_set)[0].SetData({
        gfx::Resource::SubResource(
            reinterpret_cast<Data::ConstRawPtr>(m_sp_text_mesh->GetVertices().data()), vertices_data_size,
            gfx::Resource::SubResource::Index(), gfx::Resource::BytesRange(0u, vertices_data_size)
        )
    });

    // Update index buffer
    const Data::Size indices_data_size = m_sp_text_mesh->GetIndicesDataSize();
    assert(indices_data_size);
    if (!indices_data_size)
        return;

    if (!m_sp_index_buffer || m_sp_index_buffer->GetDataSize() < indices_data_size)
    {
        const Data::Size index_buffer_size = vertices_data_size * m_settings.mesh_buffers_reservation_multiplier;
        m_sp_index_buffer = gfx::Buffer::CreateIndexBuffer(GetUIContext().GetRenderContext(), index_buffer_size, gfx::PixelFormat::R16Uint);
        m_sp_index_buffer->SetName(m_settings.name + " Text Index Buffer");
    }
    m_sp_index_buffer->SetData({
        gfx::Resource::SubResource(
            reinterpret_cast<Data::ConstRawPtr>(m_sp_text_mesh->GetIndices().data()), indices_data_size,
            gfx::Resource::SubResource::Index(), gfx::Resource::BytesRange(0u, indices_data_size)
        )
    });
}

void Text::UpdateUniformsBuffer()
{
    META_FUNCTION_TASK();
    if (m_settings.text.empty())
        return;

    if (!m_viewport_rect.size)
        throw std::logic_error("Text uniforms buffer can not be updated when one of viewport dimensions is zero.");

    gfx::Matrix44f scale_text_matrix;
    cml::matrix_scale_2D(scale_text_matrix, 2.f / m_viewport_rect.size.width, 2.f / m_viewport_rect.size.height);

    gfx::Matrix44f translate_text_matrix;
    cml::matrix_translation_2D(translate_text_matrix, -1.f, 1.f);

    Uniforms uniforms{
        scale_text_matrix * translate_text_matrix
    };

    const Data::Size uniforms_data_size = static_cast<Data::Size>(sizeof(uniforms));

    if (!m_sp_uniforms_buffer)
    {
        m_sp_uniforms_buffer = gfx::Buffer::CreateConstantBuffer(GetUIContext().GetRenderContext(), gfx::Buffer::GetAlignedBufferSize(uniforms_data_size));
        m_sp_uniforms_buffer->SetName(m_settings.name + " Text Uniforms Buffer");
    }
    m_sp_uniforms_buffer->SetData({ { reinterpret_cast<Data::ConstRawPtr>(&uniforms), uniforms_data_size } });
}

void Text::UpdateConstantsBuffer()
{
    META_FUNCTION_TASK();
    Constants constants{
        m_settings.color
    };
    const Data::Size const_data_size = static_cast<Data::Size>(sizeof(constants));

    if (!m_sp_const_buffer)
    {
        m_sp_const_buffer = gfx::Buffer::CreateConstantBuffer(GetUIContext().GetRenderContext(), gfx::Buffer::GetAlignedBufferSize(const_data_size));
        m_sp_const_buffer->SetName(m_settings.name + " Text Constants Buffer");
    }
    m_sp_const_buffer->SetData({
        gfx::Resource::SubResource(reinterpret_cast<Data::ConstRawPtr>(&constants), const_data_size)
    });
}

} // namespace Methane::Graphics