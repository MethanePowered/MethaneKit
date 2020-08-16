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
#include <Methane/Data/Math.hpp>
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
            settings.layout,
            settings.color
        }
    )
{
    META_FUNCTION_TASK();
}

Text::Text(Context& ui_context, Font& font, SettingsUtf32 settings)
    : Item(ui_context, settings.rect)
    , m_settings(std::move(settings))
    , m_sp_font(font.shared_from_this())
{
    META_FUNCTION_TASK();
    m_sp_font->Connect(*this);

    const gfx::RenderContext::Settings& context_settings = GetUIContext().GetRenderContext().GetSettings();
    m_frame_rect = GetUIContext().ConvertToPixels(m_settings.rect);

    SetRelOrigin(m_settings.rect.GetUnitOrigin());
    UpdateTextMesh();
    UpdateConstantsBuffer();

    const FrameRect viewport_rect = m_sp_text_mesh ? GetAlignedViewportRect() : m_frame_rect;

    const std::string state_name = m_settings.name + " Screen-Quad Shading";
    m_sp_state = std::dynamic_pointer_cast<gfx::RenderState>(ui_context.GetGraphicsObjectFromCache(state_name));
    if (!m_sp_state)
    {
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
                    { { gfx::Shader::Type::Vertex, "g_uniforms"  }, gfx::Program::Argument::Modifiers::None     },
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
        state_settings.viewports                                            = { gfx::GetFrameViewport(viewport_rect) };
        state_settings.scissor_rects                                        = { gfx::GetFrameScissorRect(viewport_rect) };
        state_settings.depth.enabled                                        = false;
        state_settings.depth.write_enabled                                  = false;
        state_settings.rasterizer.is_front_counter_clockwise                = true;
        state_settings.blending.render_targets[0].blend_enabled             = true;
        state_settings.blending.render_targets[0].source_rgb_blend_factor   = gfx::RenderState::Blending::Factor::SourceAlpha;
        state_settings.blending.render_targets[0].dest_rgb_blend_factor     = gfx::RenderState::Blending::Factor::OneMinusSourceAlpha;
        state_settings.blending.render_targets[0].source_alpha_blend_factor = gfx::RenderState::Blending::Factor::Zero;
        state_settings.blending.render_targets[0].dest_alpha_blend_factor   = gfx::RenderState::Blending::Factor::Zero;

        m_sp_state = gfx::RenderState::Create(GetUIContext().GetRenderContext(), state_settings);
        m_sp_state->SetName(state_name);

        if (!ui_context.AddGraphicsObjectToCache(*m_sp_state))
            throw std::logic_error("Graphics object with same name already exists in UI context cache");
    }

    const std::string sampler_name = m_settings.name + " Screen-Quad Texture Sampler";
    m_sp_atlas_sampler = std::dynamic_pointer_cast<gfx::Sampler>(ui_context.GetGraphicsObjectFromCache(sampler_name));
    if (!m_sp_atlas_sampler)
    {
        m_sp_atlas_sampler = gfx::Sampler::Create(GetUIContext().GetRenderContext(), {
            { gfx::Sampler::Filter::MinMag::Linear },
            { gfx::Sampler::Address::Mode::ClampToZero },
        });
        m_sp_atlas_sampler->SetName(sampler_name);

        if (!ui_context.AddGraphicsObjectToCache(*m_sp_atlas_sampler))
            throw std::logic_error("Graphics object with same name already exists in UI context cache");
    }

    if (m_sp_text_mesh)
    {
        InitializeFrameResources();
    }
    Item::SetRect(m_frame_rect);
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
    const bool             text_changed  = m_settings.text != text;
    const UpdateRectResult update_result = UpdateRect(ui_rect, text_changed);
    if (!text_changed && (!update_result.rect_changed || m_settings.text.empty()))
        return;

    m_settings.text = text;

    if (text_changed || update_result.size_changed)
    {
        UpdateTextMesh();
    }

    if (m_frame_resources.empty())
        return;

    FrameResources& frame_resources = GetCurrentFrameResources();
    if (!frame_resources.IsAtlasInitialized())
    {
        // If atlas texture was not initialized it has to be requested for current context first to be properly updated in future
        frame_resources.UpdateAtlasTexture(m_sp_font->GetAtlasTexturePtr(GetUIContext().GetRenderContext()));
    }

    m_is_viewport_dirty = true;
    Item::SetRect(GetUIContext().ConvertToPixels(m_frame_rect));
}

bool Text::SetRect(const UnitRect& ui_rect)
{
    META_FUNCTION_TASK();
    const UpdateRectResult update_result = UpdateRect(ui_rect, false);
    if (!update_result.rect_changed)
        return false;

    if (update_result.size_changed)
    {
        UpdateTextMesh();
    }

    m_is_viewport_dirty = true;
    return Item::SetRect(GetUIContext().ConvertToPixels(m_frame_rect));
}

Text::UpdateRectResult Text::UpdateRect(const UnitRect& ui_rect, bool reset_content_rect)
{
    META_FUNCTION_TASK();
    const UnitRect& ui_curr_rect_px  = GetRectInPixels();
    const UnitRect  ui_rect_in_units = GetUIContext().ConvertToUnits(ui_rect, m_settings.rect.units);
    const UnitRect  ui_rect_in_px    = GetUIContext().ConvertToPixels(ui_rect);
    const bool      ui_rect_changed  = ui_curr_rect_px != ui_rect_in_px;
    const bool      ui_size_changed  = ui_rect_changed && ui_curr_rect_px.size != ui_rect_in_px.size;

    m_settings.rect.origin = ui_rect_in_units.origin;
    if (ui_size_changed)
        m_settings.rect.size = ui_rect_in_units.size;

    if (reset_content_rect || ui_size_changed)
        m_frame_rect = ui_rect_in_px;
    else
        m_frame_rect.origin = ui_rect_in_px.origin;

    return { ui_rect_changed, ui_size_changed };
}

void Text::SetColor(const gfx::Color4f& color)
{
    META_FUNCTION_TASK();
    if (m_settings.color == color)
        return;

    m_settings.color = color;
    UpdateConstantsBuffer();
}

void Text::SetLayout(const Layout& layout)
{
    META_FUNCTION_TASK();
    if (m_settings.layout == layout)
        return;

    m_settings.layout = layout;

    UpdateTextMesh();

    m_is_viewport_dirty = true;
    Item::SetRect(GetUIContext().ConvertToPixels(m_frame_rect));
}

void Text::SetWrap(Wrap wrap)
{
    META_FUNCTION_TASK();
    Layout layout = m_settings.layout;
    layout.wrap = wrap;
    SetLayout(layout);
}

void Text::SetHorizontalAlignment(HorizontalAlignment alignment)
{
    META_FUNCTION_TASK();
    Layout layout = m_settings.layout;
    layout.horizontal_alignment = alignment;
    SetLayout(layout);
}

void Text::SetVerticalAlignment(VerticalAlignment alignment)
{
    META_FUNCTION_TASK();
    Layout layout = m_settings.layout;
    layout.vertical_alignment = alignment;
    SetLayout(layout);
}

void Text::Update(const gfx::FrameSize& render_attachment_size)
{
    META_FUNCTION_TASK();
    if (m_frame_resources.empty())
        return;

    FrameResources& frame_resources = GetCurrentFrameResources();

    if (m_is_viewport_dirty)
    {
        UpdateViewport(render_attachment_size);
    }
    if (frame_resources.IsDirty(FrameResources::Dirty::Mesh) && m_sp_text_mesh)
    {
        frame_resources.UpdateMeshBuffers(GetUIContext().GetRenderContext(), *m_sp_text_mesh, m_settings.name, m_settings.mesh_buffers_reservation_multiplier);
    }
    if (frame_resources.IsDirty(FrameResources::Dirty::Atlas) && m_sp_font)
    {
        if (!frame_resources.UpdateAtlasTexture(m_sp_font->GetAtlasTexturePtr(GetUIContext().GetRenderContext())) && m_sp_state)
        {
            frame_resources.InitializeProgramBindings(*m_sp_state, m_sp_const_buffer, m_sp_atlas_sampler);
        }
    }
    if (frame_resources.IsDirty(FrameResources::Dirty::Uniforms) && m_sp_text_mesh)
    {
        frame_resources.UpdateUniformsBuffer(GetUIContext().GetRenderContext(), *m_sp_text_mesh, m_settings.name);
    }
    assert(!frame_resources.IsDirty() || !m_sp_text_mesh || !m_sp_font || !m_sp_text_mesh);
}

void Text::Draw(gfx::RenderCommandList& cmd_list, gfx::CommandList::DebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    if (m_frame_resources.empty())
        return;

    FrameResources& frame_resources = GetCurrentFrameResources();
    if (!frame_resources.IsInitialized())
        return;

    cmd_list.Reset(m_sp_state, p_debug_group);
    cmd_list.SetProgramBindings(frame_resources.GetProgramBindings());
    cmd_list.SetVertexBuffers(frame_resources.GetVertexBufferSet());
    cmd_list.DrawIndexed(gfx::RenderCommandList::Primitive::Triangle, frame_resources.GetIndexBuffer());
}

void Text::OnFontAtlasTextureReset(Font& font, const Ptr<gfx::Texture>& sp_old_atlas_texture, const Ptr<gfx::Texture>& sp_new_atlas_texture)
{
    META_FUNCTION_TASK();
    META_UNUSED(sp_old_atlas_texture);
    if (m_sp_font.get() != std::addressof(font) || m_frame_resources.empty() ||
        (sp_new_atlas_texture && std::addressof(GetUIContext().GetRenderContext()) != std::addressof(sp_new_atlas_texture->GetContext())))
        return;

    MakeFrameResourcesDirty(FrameResources::Dirty::Atlas);

    if (m_sp_text_mesh)
    {
        // Reset text mesh along with font atlas for texture coordinates in mesh to match atlas dimensions
        m_sp_text_mesh.reset();
        UpdateTextMesh();
    }

    if (GetUIContext().GetRenderContext().IsCompletingInitialization())
    {
        // If font atlas was auto-updated on context initialization complete,
        // the atlas texture and mesh buffers need to be updated now for current frame rendering
        Update(m_render_attachment_size);
    }
}

Text::FrameResources::FrameResources(gfx::RenderState& state, gfx::RenderContext& render_context,
                                     const Ptr<gfx::Buffer>& sp_const_buffer, const Ptr<gfx::Texture>& sp_atlas_texture, const Ptr<gfx::Sampler>& sp_atlas_sampler,
                                     const TextMesh& text_mesh, const std::string& text_name, Data::Size reservation_multiplier)
     : m_sp_atlas_texture(sp_atlas_texture)
{
    META_FUNCTION_TASK();
    UpdateMeshBuffers(render_context, text_mesh, text_name, reservation_multiplier);
    UpdateUniformsBuffer(render_context, text_mesh, text_name);
    InitializeProgramBindings(state, sp_const_buffer, sp_atlas_sampler);
}

void Text::FrameResources::InitializeProgramBindings(gfx::RenderState& state, const Ptr<gfx::Buffer>& sp_const_buffer, const Ptr<gfx::Sampler>& sp_atlas_sampler)
{
    META_FUNCTION_TASK();
    if (m_sp_program_bindings)
        return;

    if (!sp_const_buffer || !sp_atlas_sampler)
        throw std::invalid_argument("Can not create text program bindings. Font atlas sampler or constant buffer is not initialized.");

    if (!m_sp_atlas_texture || !m_sp_uniforms_buffer)
        throw std::logic_error("Can not create text program bindings. Font atlas texture or uniforms buffer is not initialized.");

    m_sp_program_bindings = gfx::ProgramBindings::Create(state.GetSettings().sp_program, {
        { { gfx::Shader::Type::Vertex, "g_uniforms"  }, { { m_sp_uniforms_buffer } } },
        { { gfx::Shader::Type::Pixel,  "g_constants" }, { { sp_const_buffer      } } },
        { { gfx::Shader::Type::Pixel,  "g_texture"   }, { { m_sp_atlas_texture   } } },
        { { gfx::Shader::Type::Pixel,  "g_sampler"   }, { { sp_atlas_sampler     } } },
    });
}

gfx::BufferSet& Text::FrameResources::GetVertexBufferSet() const
{
    if (!m_sp_vertex_buffer_set)
        throw std::logic_error("Text vertex buffers are not initialized.");

    return *m_sp_vertex_buffer_set;
}

gfx::Buffer& Text::FrameResources::GetIndexBuffer() const
{
    if (!m_sp_index_buffer)
        throw std::logic_error("Text index buffer are not initialized.");

    return *m_sp_index_buffer;
}

gfx::ProgramBindings& Text::FrameResources::GetProgramBindings() const
{
    if (!m_sp_program_bindings)
        throw std::logic_error("Text program bindings are not initialized.");

    return *m_sp_program_bindings;
}

bool Text::FrameResources::UpdateAtlasTexture(const Ptr<gfx::Texture>& sp_new_atlas_texture)
{
    META_FUNCTION_TASK();
    if (m_sp_atlas_texture.get() == sp_new_atlas_texture.get())
        return true;

    m_sp_atlas_texture = sp_new_atlas_texture;

    if (!m_sp_atlas_texture)
    {
        m_sp_program_bindings.reset();
        return true;
    }

    if (!m_sp_program_bindings)
        return false;

    m_sp_program_bindings->Get({ gfx::Shader::Type::Pixel, "g_texture" })->SetResourceLocations({ { m_sp_atlas_texture } });
    m_dirty_mask &= ~Dirty::Atlas;
    return true;
}

void Text::FrameResources::UpdateMeshBuffers(gfx::RenderContext& render_context, const TextMesh& text_mesh,
                                             const std::string& text_name, Data::Size reservation_multiplier)
{
    META_FUNCTION_TASK();

    // Update vertex buffer
    const Data::Size vertices_data_size = text_mesh.GetVerticesDataSize();
    assert(vertices_data_size);
    if (!vertices_data_size)
    {
        m_sp_index_buffer.reset();
        m_sp_vertex_buffer_set.reset();
        return;
    }
    
    if (!m_sp_vertex_buffer_set || (*m_sp_vertex_buffer_set)[0].GetDataSize() < vertices_data_size)
    {
        const Data::Size vertex_buffer_size = vertices_data_size * reservation_multiplier;
        Ptr<gfx::Buffer> sp_vertex_buffer = gfx::Buffer::CreateVertexBuffer(render_context, vertex_buffer_size, text_mesh.GetVertexSize());
        sp_vertex_buffer->SetName(text_name + " Text Vertex Buffer");
        m_sp_vertex_buffer_set = gfx::BufferSet::CreateVertexBuffers({ *sp_vertex_buffer });
    }
    (*m_sp_vertex_buffer_set)[0].SetData({
        gfx::Resource::SubResource(
            reinterpret_cast<Data::ConstRawPtr>(text_mesh.GetVertices().data()), vertices_data_size,
            gfx::Resource::SubResource::Index(), gfx::Resource::BytesRange(0u, vertices_data_size)
        )
    });

    // Update index buffer
    const Data::Size indices_data_size = text_mesh.GetIndicesDataSize();
    assert(indices_data_size);
    if (!indices_data_size)
    {
        m_sp_index_buffer.reset();
        return;
    }

    if (!m_sp_index_buffer || m_sp_index_buffer->GetDataSize() < indices_data_size)
    {
        const Data::Size index_buffer_size = vertices_data_size * reservation_multiplier;
        m_sp_index_buffer = gfx::Buffer::CreateIndexBuffer(render_context, index_buffer_size, gfx::PixelFormat::R16Uint);
        m_sp_index_buffer->SetName(text_name + " Text Index Buffer");
    }

    m_sp_index_buffer->SetData({
        gfx::Resource::SubResource(
            reinterpret_cast<Data::ConstRawPtr>(text_mesh.GetIndices().data()), indices_data_size,
            gfx::Resource::SubResource::Index(), gfx::Resource::BytesRange(0u, indices_data_size)
        )
    });

    m_dirty_mask &= ~Dirty::Mesh;
}

void Text::FrameResources::UpdateUniformsBuffer(gfx::RenderContext& render_context, const TextMesh& text_mesh, const std::string& text_name)
{
    META_FUNCTION_TASK();
    const gfx::FrameSize& content_size = text_mesh.GetContentSize();
    if (!content_size)
        throw std::logic_error("Text uniforms buffer can not be updated when one of content size dimensions is zero.");

    gfx::Matrix44f scale_text_matrix;
    cml::matrix_scale_2D(scale_text_matrix, 2.f / content_size.width, 2.f / content_size.height);

    gfx::Matrix44f translate_text_matrix;
    cml::matrix_translation_2D(translate_text_matrix, -1.f, 1.f);

    Uniforms uniforms{
        scale_text_matrix * translate_text_matrix
    };

    const Data::Size uniforms_data_size = static_cast<Data::Size>(sizeof(uniforms));

    if (!m_sp_uniforms_buffer)
    {
        m_sp_uniforms_buffer = gfx::Buffer::CreateConstantBuffer(render_context, gfx::Buffer::GetAlignedBufferSize(uniforms_data_size));
        m_sp_uniforms_buffer->SetName(text_name + " Text Uniforms Buffer");

        if (m_sp_program_bindings)
        {
            m_sp_program_bindings->Get({ gfx::Shader::Type::Vertex, "g_uniforms" })->SetResourceLocations({ { m_sp_uniforms_buffer } });
        }
    }
    m_sp_uniforms_buffer->SetData({ { reinterpret_cast<Data::ConstRawPtr>(&uniforms), uniforms_data_size } });

    m_dirty_mask &= ~Dirty::Uniforms;
}

void Text::InitializeFrameResources()
{
    META_FUNCTION_TASK();
    if (!m_frame_resources.empty())
        throw std::logic_error("Frame resources have been already initialized.");

    if (!m_sp_state)
        throw std::logic_error("Text render state is not initialized.");

    if (!m_sp_text_mesh)
        throw std::logic_error("Text mesh is not initialized.");

    gfx::RenderContext& render_context = GetUIContext().GetRenderContext();
    const uint32_t frame_buffers_count = render_context.GetSettings().frame_buffers_count;
    m_frame_resources.reserve(frame_buffers_count);

    const Ptr<gfx::Texture>& sp_atlas_texture = m_sp_font->GetAtlasTexturePtr(render_context);
    for(uint32_t frame_buffer_index = 0u; frame_buffer_index < frame_buffers_count; ++frame_buffer_index)
    {
        m_frame_resources.emplace_back(
            *m_sp_state, render_context, m_sp_const_buffer, sp_atlas_texture, m_sp_atlas_sampler,
            *m_sp_text_mesh, m_settings.name, m_settings.mesh_buffers_reservation_multiplier
        );
    }
}

Text::FrameResources& Text::GetCurrentFrameResources()
{
    META_FUNCTION_TASK();
    const uint32_t frame_index = GetUIContext().GetRenderContext().GetFrameBufferIndex();
    if (frame_index >= m_frame_resources.size())
        throw std::logic_error("No resources available for current frame buffer index.");

    return m_frame_resources[frame_index];
}

void Text::MakeFrameResourcesDirty(FrameResources::Dirty::Mask dirty_flags)
{
    META_FUNCTION_TASK();
    for(FrameResources& frame_resources : m_frame_resources)
    {
        frame_resources.SetDirty(dirty_flags);
    }
}

void Text::UpdateTextMesh()
{
    META_FUNCTION_TASK();
    if (m_settings.text.empty())
    {
        m_frame_resources.clear();
        m_sp_text_mesh.reset();
        return;
    }

    // Fill font with new text chars strictly before building the text mesh, to be sure that font atlas size is up to date
    m_sp_font->AddChars(m_settings.text);

    if (!m_sp_font->GetAtlasSize())
        return;

    if (m_settings.incremental_update && m_sp_text_mesh &&
        m_sp_text_mesh->IsUpdatable(m_settings.text, m_settings.layout, *m_sp_font, m_frame_rect.size))
    {
        m_sp_text_mesh->Update(m_settings.text, m_frame_rect.size);
    }
    else
    {
        m_sp_text_mesh = std::make_unique<TextMesh>(m_settings.text, m_settings.layout, *m_sp_font, m_frame_rect.size);
    }

    if (m_frame_resources.empty() && m_sp_state)
    {
        InitializeFrameResources();
        return;
    }
    
    MakeFrameResourcesDirty(FrameResources::Dirty::Mesh | FrameResources::Dirty::Uniforms);
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

FrameRect Text::GetAlignedViewportRect()
{
    META_FUNCTION_TASK();
    if (!m_sp_text_mesh)
        throw std::logic_error("Text mesh must be initialized.");

    FrameSize content_size = m_sp_text_mesh->GetContentSize();
    if (!content_size)
        throw std::logic_error("All dimension of text content size should be non-zero.");

    if (!m_frame_rect.size)
        throw std::logic_error("All dimension of frame size should be non-zero.");

    // Position viewport rect inside frame rect based on text alignment
    FrameRect viewport_rect(m_frame_rect.origin, content_size);

    if (m_settings.adjust_vertical_content_offset)
    {
        // Apply vertical offset to make top of content match the rect top coordinate
        const uint32_t content_top_offset = m_sp_text_mesh->GetContentTopOffset();
        assert(content_top_offset <= content_size.height);
        content_size.height -= content_top_offset;
        viewport_rect.origin.SetY(m_frame_rect.origin.GetY() - content_top_offset);
    }

    if (content_size.width != m_frame_rect.size.width)
    {
        switch (m_settings.layout.horizontal_alignment)
        {
        case HorizontalAlignment::Left:   break;
        case HorizontalAlignment::Right:  viewport_rect.origin.SetX(viewport_rect.origin.GetX() + static_cast<int32_t>(m_frame_rect.size.width - content_size.width)); break;
        case HorizontalAlignment::Center: viewport_rect.origin.SetX(viewport_rect.origin.GetX() + static_cast<int32_t>(m_frame_rect.size.width - content_size.width) / 2); break;
        }
    }
    if (content_size.height != m_frame_rect.size.height)
    {
        switch (m_settings.layout.vertical_alignment)
        {
        case VerticalAlignment::Top:      break;
        case VerticalAlignment::Bottom:   viewport_rect.origin.SetY(viewport_rect.origin.GetY() + static_cast<int32_t>(m_frame_rect.size.height - content_size.height)); break;
        case VerticalAlignment::Center:   viewport_rect.origin.SetY(viewport_rect.origin.GetY() + static_cast<int32_t>(m_frame_rect.size.height - content_size.height) / 2); break;
        }
    }

    return viewport_rect;
}

void Text::UpdateViewport(const gfx::FrameSize& render_attachment_size)
{
    META_FUNCTION_TASK();
    if (!m_sp_text_mesh)
        return;

    const FrameRect viewport_rect = GetAlignedViewportRect();
    m_sp_state->SetViewports({ gfx::GetFrameViewport(viewport_rect) });
    m_sp_state->SetScissorRects({ gfx::GetFrameScissorRect(viewport_rect, render_attachment_size) });
}

std::string Text::GetWrapName(Wrap wrap) noexcept
{
    META_FUNCTION_TASK();
    switch (wrap)
    {
    case Wrap::None:     return "No Wrap";
    case Wrap::Anywhere: return "Wrap Anywhere";
    case Wrap::Word:     return "Wrap Words";
    }
    return "Undefined Wrap";
}

std::string Text::GetHorizontalAlignmentName(HorizontalAlignment alignment) noexcept
{
    META_FUNCTION_TASK();
    switch(alignment)
    {
    case HorizontalAlignment::Left:   return "Left";
    case HorizontalAlignment::Right:  return "Right";
    case HorizontalAlignment::Center: return "Center";
    }
    return "Undefined Alignment";
}

std::string Text::GetVerticalAlignmentName(VerticalAlignment alignment) noexcept
{
    META_FUNCTION_TASK();
    switch(alignment)
    {
    case VerticalAlignment::Top:    return "Top";
    case VerticalAlignment::Bottom: return "Bottom";
    case VerticalAlignment::Center: return "Center";
    }
    return "Undefined Alignment";
}

} // namespace Methane::Graphics
