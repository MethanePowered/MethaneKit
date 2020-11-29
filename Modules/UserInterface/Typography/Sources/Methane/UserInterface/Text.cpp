/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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
#include <Methane/Checks.hpp>

#include <cml/mathlib/mathlib.h>
#include <magic_enum.hpp>

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
    , m_font_ptr(font.shared_from_this())
{
    META_FUNCTION_TASK();
    m_font_ptr->Connect(*this);

    const gfx::RenderContext::Settings& context_settings = GetUIContext().GetRenderContext().GetSettings();
    m_frame_rect = GetUIContext().ConvertToPixels(m_settings.rect);

    SetRelOrigin(m_settings.rect.GetUnitOrigin());
    UpdateTextMesh();
    UpdateConstantsBuffer();

    const FrameRect viewport_rect = m_text_mesh_ptr ? GetAlignedViewportRect() : m_frame_rect.AsRect();
    gfx::Object::Registry& gfx_objects_registry = ui_context.GetRenderContext().GetObjectsRegistry();

    static const std::string s_state_name = "Text Render State";
    m_render_state_ptr = std::dynamic_pointer_cast<gfx::RenderState>(gfx_objects_registry.GetGraphicsObject(s_state_name));
    if (!m_render_state_ptr)
    {
        gfx::RenderState::Settings state_settings;
        state_settings.program_ptr = gfx::Program::Create(GetUIContext().GetRenderContext(),
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
                    { { gfx::Shader::Type::Pixel,  "g_constants" }, gfx::Program::Argument::Modifiers::None     },
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
        state_settings.program_ptr->SetName("Text Shading");
        state_settings.depth.enabled                                        = false;
        state_settings.depth.write_enabled                                  = false;
        state_settings.rasterizer.is_front_counter_clockwise                = true;
        state_settings.blending.render_targets[0].blend_enabled             = true;
        state_settings.blending.render_targets[0].source_rgb_blend_factor   = gfx::RenderState::Blending::Factor::SourceAlpha;
        state_settings.blending.render_targets[0].dest_rgb_blend_factor     = gfx::RenderState::Blending::Factor::OneMinusSourceAlpha;
        state_settings.blending.render_targets[0].source_alpha_blend_factor = gfx::RenderState::Blending::Factor::Zero;
        state_settings.blending.render_targets[0].dest_alpha_blend_factor   = gfx::RenderState::Blending::Factor::Zero;

        m_render_state_ptr = gfx::RenderState::Create(GetUIContext().GetRenderContext(), state_settings);
        m_render_state_ptr->SetName(s_state_name);

        gfx_objects_registry.AddGraphicsObject(*m_render_state_ptr);
    }

    m_view_state_ptr = gfx::ViewState::Create({
        { gfx::GetFrameViewport(viewport_rect)    },
        { gfx::GetFrameScissorRect(viewport_rect) }
    });

    static const std::string s_sampler_name = "Font Atlas Sampler";
    m_atlas_sampler_ptr = std::dynamic_pointer_cast<gfx::Sampler>(gfx_objects_registry.GetGraphicsObject(s_sampler_name));
    if (!m_atlas_sampler_ptr)
    {
        m_atlas_sampler_ptr = gfx::Sampler::Create(GetUIContext().GetRenderContext(), {
            gfx::Sampler::Filter(gfx::Sampler::Filter::MinMag::Linear),
            gfx::Sampler::Address(gfx::Sampler::Address::Mode::ClampToZero),
        });
        m_atlas_sampler_ptr->SetName(s_sampler_name);

        gfx_objects_registry.AddGraphicsObject(*m_atlas_sampler_ptr);
    }

    if (m_text_mesh_ptr)
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
    m_font_ptr->Disconnect(*this);
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
        frame_resources.UpdateAtlasTexture(m_font_ptr->GetAtlasTexturePtr(GetUIContext().GetRenderContext()));
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
    Item::SetRect(UnitRect(Units::Pixels, m_frame_rect));
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
    if (frame_resources.IsDirty(FrameResources::DirtyFlags::Mesh) && m_text_mesh_ptr)
    {
        frame_resources.UpdateMeshBuffers(GetUIContext().GetRenderContext(), *m_text_mesh_ptr, m_settings.name, m_settings.mesh_buffers_reservation_multiplier);
    }
    if (frame_resources.IsDirty(FrameResources::DirtyFlags::Atlas) && m_font_ptr &&
        !frame_resources.UpdateAtlasTexture(m_font_ptr->GetAtlasTexturePtr(GetUIContext().GetRenderContext())) && m_render_state_ptr)
    {
        frame_resources.InitializeProgramBindings(*m_render_state_ptr, m_const_buffer_ptr, m_atlas_sampler_ptr);
    }
    if (frame_resources.IsDirty(FrameResources::DirtyFlags::Uniforms) && m_text_mesh_ptr)
    {
        frame_resources.UpdateUniformsBuffer(GetUIContext().GetRenderContext(), *m_text_mesh_ptr, m_settings.name);
    }
    assert(!frame_resources.IsDirty() || !m_text_mesh_ptr || !m_font_ptr || !m_text_mesh_ptr);
}

void Text::Draw(gfx::RenderCommandList& cmd_list, gfx::CommandList::DebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    if (m_frame_resources.empty())
        return;

    const FrameResources& frame_resources = GetCurrentFrameResources();
    if (!frame_resources.IsInitialized())
        return;

    cmd_list.ResetWithState(m_render_state_ptr, p_debug_group);
    cmd_list.SetViewState(*m_view_state_ptr);
    cmd_list.SetProgramBindings(frame_resources.GetProgramBindings());
    cmd_list.SetVertexBuffers(frame_resources.GetVertexBufferSet());
    cmd_list.DrawIndexed(gfx::RenderCommandList::Primitive::Triangle, frame_resources.GetIndexBuffer());
}

void Text::OnFontAtlasTextureReset(Font& font, const Ptr<gfx::Texture>& old_atlas_texture_ptr, const Ptr<gfx::Texture>& new_atlas_texture_ptr)
{
    META_FUNCTION_TASK();
    META_UNUSED(old_atlas_texture_ptr);
    if (m_font_ptr.get() != std::addressof(font) || m_frame_resources.empty() ||
        (new_atlas_texture_ptr && std::addressof(GetUIContext().GetRenderContext()) != std::addressof(new_atlas_texture_ptr->GetContext())))
        return;

    MakeFrameResourcesDirty(FrameResources::DirtyFlags::Atlas);

    if (m_text_mesh_ptr)
    {
        // Reset text mesh along with font atlas for texture coordinates in mesh to match atlas dimensions
        m_text_mesh_ptr.reset();
        UpdateTextMesh();
    }

    if (GetUIContext().GetRenderContext().IsCompletingInitialization())
    {
        // If font atlas was auto-updated on context initialization complete,
        // the atlas texture and mesh buffers need to be updated now for current frame rendering
        Update(m_render_attachment_size);
    }
}

Text::FrameResources::FrameResources(const gfx::RenderState& state, gfx::RenderContext& render_context,
                                     const Ptr<gfx::Buffer>& const_buffer_ptr, const Ptr<gfx::Texture>& atlas_texture_ptr, const Ptr<gfx::Sampler>& atlas_sampler_ptr,
                                     const TextMesh& text_mesh, const std::string& text_name, Data::Size reservation_multiplier)
     : m_atlas_texture_ptr(atlas_texture_ptr)
{
    META_FUNCTION_TASK();
    UpdateMeshBuffers(render_context, text_mesh, text_name, reservation_multiplier);
    UpdateUniformsBuffer(render_context, text_mesh, text_name);
    InitializeProgramBindings(state, const_buffer_ptr, atlas_sampler_ptr);
}

void Text::FrameResources::SetDirty(DirtyFlags dirty_flags) noexcept
{
    using namespace magic_enum::bitwise_operators;
    m_dirty_mask |= dirty_flags;
}

bool Text::FrameResources::IsDirty(DirtyFlags dirty_flags) const noexcept
{
    using namespace magic_enum::bitwise_operators;
    return magic_enum::flags::enum_contains(m_dirty_mask & dirty_flags);
}

void Text::FrameResources::InitializeProgramBindings(const gfx::RenderState& state, const Ptr<gfx::Buffer>& const_buffer_ptr, const Ptr<gfx::Sampler>& atlas_sampler_ptr)
{
    META_FUNCTION_TASK();
    if (m_program_bindings_ptr)
        return;

    META_CHECK_ARG_NOT_NULL(const_buffer_ptr);
    META_CHECK_ARG_NOT_NULL(atlas_sampler_ptr);
    META_CHECK_ARG_NOT_NULL(m_atlas_texture_ptr);
    META_CHECK_ARG_NOT_NULL(m_uniforms_buffer_ptr);

    m_program_bindings_ptr = gfx::ProgramBindings::Create(state.GetSettings().program_ptr, {
        { { gfx::Shader::Type::Vertex, "g_uniforms"  }, { { m_uniforms_buffer_ptr } } },
        { { gfx::Shader::Type::Pixel,  "g_constants" }, { { const_buffer_ptr      } } },
        { { gfx::Shader::Type::Pixel,  "g_texture"   }, { { m_atlas_texture_ptr   } } },
        { { gfx::Shader::Type::Pixel,  "g_sampler"   }, { { atlas_sampler_ptr     } } },
    });
}

gfx::BufferSet& Text::FrameResources::GetVertexBufferSet() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL_DESCR(m_vertex_buffer_set_ptr, "text vertex buffers are not initialized");
    return *m_vertex_buffer_set_ptr;
}

gfx::Buffer& Text::FrameResources::GetIndexBuffer() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL_DESCR(m_index_buffer_ptr, "text index buffer are not initialized");
    return *m_index_buffer_ptr;
}

gfx::ProgramBindings& Text::FrameResources::GetProgramBindings() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL_DESCR(m_program_bindings_ptr, "text program bindings are not initialized");
    return *m_program_bindings_ptr;
}

bool Text::FrameResources::UpdateAtlasTexture(const Ptr<gfx::Texture>& new_atlas_texture_ptr)
{
    META_FUNCTION_TASK();
    if (m_atlas_texture_ptr.get() == new_atlas_texture_ptr.get())
        return true;

    m_atlas_texture_ptr = new_atlas_texture_ptr;

    if (!m_atlas_texture_ptr)
    {
        m_program_bindings_ptr.reset();
        return true;
    }

    if (!m_program_bindings_ptr)
        return false;

    m_program_bindings_ptr->Get({ gfx::Shader::Type::Pixel, "g_texture" })->SetResourceLocations({ { m_atlas_texture_ptr } });

    using namespace magic_enum::bitwise_operators;
    m_dirty_mask &= ~DirtyFlags::Atlas;
    return true;
}

void Text::FrameResources::UpdateMeshBuffers(gfx::RenderContext& render_context, const TextMesh& text_mesh,
                                             const std::string& text_name, Data::Size reservation_multiplier)
{
    META_FUNCTION_TASK();

    // Update vertex buffer
    const Data::Size vertices_data_size = text_mesh.GetVerticesDataSize();
    META_CHECK_ARG_NOT_ZERO(vertices_data_size);
    
    if (!m_vertex_buffer_set_ptr || (*m_vertex_buffer_set_ptr)[0].GetDataSize() < vertices_data_size)
    {
        const Data::Size vertex_buffer_size = vertices_data_size * reservation_multiplier;
        Ptr<gfx::Buffer> vertex_buffer_ptr = gfx::Buffer::CreateVertexBuffer(render_context, vertex_buffer_size, text_mesh.GetVertexSize());
        vertex_buffer_ptr->SetName(text_name + " Text Vertex Buffer");
        m_vertex_buffer_set_ptr = gfx::BufferSet::CreateVertexBuffers({ *vertex_buffer_ptr });
    }
    (*m_vertex_buffer_set_ptr)[0].SetData({
        gfx::Resource::SubResource(
            reinterpret_cast<Data::ConstRawPtr>(text_mesh.GetVertices().data()), vertices_data_size,
            gfx::Resource::SubResource::Index(), gfx::Resource::BytesRange(0U, vertices_data_size)
        )
    });

    // Update index buffer
    const Data::Size indices_data_size = text_mesh.GetIndicesDataSize();
    META_CHECK_ARG_NOT_ZERO(indices_data_size);

    if (!m_index_buffer_ptr || m_index_buffer_ptr->GetDataSize() < indices_data_size)
    {
        const Data::Size index_buffer_size = vertices_data_size * reservation_multiplier;
        m_index_buffer_ptr = gfx::Buffer::CreateIndexBuffer(render_context, index_buffer_size, gfx::PixelFormat::R16Uint);
        m_index_buffer_ptr->SetName(text_name + " Text Index Buffer");
    }

    m_index_buffer_ptr->SetData({
        gfx::Resource::SubResource(
            reinterpret_cast<Data::ConstRawPtr>(text_mesh.GetIndices().data()), indices_data_size,
            gfx::Resource::SubResource::Index(), gfx::Resource::BytesRange(0U, indices_data_size)
        )
    });

    using namespace magic_enum::bitwise_operators;
    m_dirty_mask &= ~DirtyFlags::Mesh;
}

void Text::FrameResources::UpdateUniformsBuffer(gfx::RenderContext& render_context, const TextMesh& text_mesh, const std::string& text_name)
{
    META_FUNCTION_TASK();

    const gfx::FrameSize& content_size = text_mesh.GetContentSize();
    META_CHECK_ARG_NOT_ZERO_DESCR(content_size, "text uniforms buffer can not be updated when one of content size dimensions is zero");

    gfx::Matrix44f scale_text_matrix;
    cml::matrix_scale_2D(scale_text_matrix, 2.F / static_cast<float>(content_size.width), 2.F / static_cast<float>(content_size.height));

    gfx::Matrix44f translate_text_matrix;
    cml::matrix_translation_2D(translate_text_matrix, -1.F, 1.F);

    Uniforms uniforms{
        scale_text_matrix * translate_text_matrix
    };

    const auto uniforms_data_size = static_cast<Data::Size>(sizeof(uniforms));

    if (!m_uniforms_buffer_ptr)
    {
        m_uniforms_buffer_ptr = gfx::Buffer::CreateConstantBuffer(render_context, gfx::Buffer::GetAlignedBufferSize(uniforms_data_size));
        m_uniforms_buffer_ptr->SetName(text_name + " Text Uniforms Buffer");

        if (m_program_bindings_ptr)
        {
            m_program_bindings_ptr->Get({ gfx::Shader::Type::Vertex, "g_uniforms" })->SetResourceLocations({ { m_uniforms_buffer_ptr } });
        }
    }
    m_uniforms_buffer_ptr->SetData({ { reinterpret_cast<Data::ConstRawPtr>(&uniforms), uniforms_data_size } });

    using namespace magic_enum::bitwise_operators;
    m_dirty_mask &= ~DirtyFlags::Uniforms;
}

void Text::InitializeFrameResources()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NAME_DESCR("m_frame_resources", m_frame_resources.empty(), "frame resources have been initialized already");
    META_CHECK_ARG_NOT_NULL_DESCR(m_render_state_ptr, "text render state is not initialized");
    META_CHECK_ARG_NOT_NULL_DESCR(m_text_mesh_ptr, "text mesh is not initialized");

    gfx::RenderContext& render_context = GetUIContext().GetRenderContext();
    const uint32_t frame_buffers_count = render_context.GetSettings().frame_buffers_count;
    m_frame_resources.reserve(frame_buffers_count);

    const Ptr<gfx::Texture>& atlas_texture_ptr = m_font_ptr->GetAtlasTexturePtr(render_context);
    for(uint32_t frame_buffer_index = 0U; frame_buffer_index < frame_buffers_count; ++frame_buffer_index)
    {
        m_frame_resources.emplace_back(
            *m_render_state_ptr, render_context, m_const_buffer_ptr, atlas_texture_ptr, m_atlas_sampler_ptr,
            *m_text_mesh_ptr, m_settings.name, m_settings.mesh_buffers_reservation_multiplier
        );
    }
}

Text::FrameResources& Text::GetCurrentFrameResources()
{
    META_FUNCTION_TASK();
    const uint32_t frame_index = GetUIContext().GetRenderContext().GetFrameBufferIndex();
    META_CHECK_ARG_LESS_DESCR(frame_index, m_frame_resources.size(), "no resources available for the current frame buffer index");
    return m_frame_resources[frame_index];
}

void Text::MakeFrameResourcesDirty(FrameResources::DirtyFlags dirty_flags)
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
        m_text_mesh_ptr.reset();
        return;
    }

    // Fill font with new text chars strictly before building the text mesh, to be sure that font atlas size is up to date
    m_font_ptr->AddChars(m_settings.text);

    if (!m_font_ptr->GetAtlasSize())
        return;

    if (m_settings.incremental_update && m_text_mesh_ptr &&
        m_text_mesh_ptr->IsUpdatable(m_settings.text, m_settings.layout, *m_font_ptr, m_frame_rect.size))
    {
        m_text_mesh_ptr->Update(m_settings.text, m_frame_rect.size);
    }
    else
    {
        m_text_mesh_ptr = std::make_unique<TextMesh>(m_settings.text, m_settings.layout, *m_font_ptr, m_frame_rect.size);
    }

    if (m_frame_resources.empty() && m_render_state_ptr)
    {
        InitializeFrameResources();
        return;
    }

    using namespace magic_enum::bitwise_operators;
    MakeFrameResourcesDirty(FrameResources::DirtyFlags::Mesh | FrameResources::DirtyFlags::Uniforms);
}

void Text::UpdateConstantsBuffer()
{
    META_FUNCTION_TASK();
    Constants constants{
        m_settings.color
    };
    const auto const_data_size = static_cast<Data::Size>(sizeof(constants));

    if (!m_const_buffer_ptr)
    {
        m_const_buffer_ptr = gfx::Buffer::CreateConstantBuffer(GetUIContext().GetRenderContext(), gfx::Buffer::GetAlignedBufferSize(const_data_size));
        m_const_buffer_ptr->SetName(m_settings.name + " Text Constants Buffer");
    }
    m_const_buffer_ptr->SetData({
        gfx::Resource::SubResource(reinterpret_cast<Data::ConstRawPtr>(&constants), const_data_size)
    });
}

FrameRect Text::GetAlignedViewportRect() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL_DESCR(m_text_mesh_ptr, "text mesh must be initialized");

    FrameSize content_size = m_text_mesh_ptr->GetContentSize();
    META_CHECK_ARG_NOT_ZERO_DESCR(content_size, "all dimension of text content size should be non-zero");
    META_CHECK_ARG_NOT_ZERO_DESCR(m_frame_rect.size, "all dimension of frame size should be non-zero");

    // Position viewport rect inside frame rect based on text alignment
    FrameRect viewport_rect(m_frame_rect.origin, content_size);

    if (m_settings.adjust_vertical_content_offset)
    {
        // Apply vertical offset to make top of content match the rect top coordinate
        const uint32_t content_top_offset = m_text_mesh_ptr->GetContentTopOffset();
        META_CHECK_ARG_LESS(content_top_offset, content_size.height + 1);

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
        default:                          META_UNEXPECTED_ENUM_ARG(m_settings.layout.horizontal_alignment);
        }
    }
    if (content_size.height != m_frame_rect.size.height)
    {
        switch (m_settings.layout.vertical_alignment)
        {
        case VerticalAlignment::Top:      break;
        case VerticalAlignment::Bottom:   viewport_rect.origin.SetY(viewport_rect.origin.GetY() + static_cast<int32_t>(m_frame_rect.size.height - content_size.height)); break;
        case VerticalAlignment::Center:   viewport_rect.origin.SetY(viewport_rect.origin.GetY() + static_cast<int32_t>(m_frame_rect.size.height - content_size.height) / 2); break;
        default:                          META_UNEXPECTED_ENUM_ARG(m_settings.layout.vertical_alignment);
        }
    }

    return viewport_rect;
}

void Text::UpdateViewport(const gfx::FrameSize& render_attachment_size)
{
    META_FUNCTION_TASK();
    if (!m_text_mesh_ptr)
        return;

    const FrameRect viewport_rect = GetAlignedViewportRect();
    m_view_state_ptr->SetViewports({ gfx::GetFrameViewport(viewport_rect) });
    m_view_state_ptr->SetScissorRects({ gfx::GetFrameScissorRect(viewport_rect, render_attachment_size) });
    m_is_viewport_dirty = false;
}

} // namespace Methane::Graphics
