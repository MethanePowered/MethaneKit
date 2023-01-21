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

#include <Methane/Graphics/RHI/RenderContext.h>
#include <Methane/Graphics/RHI/RenderCommandList.h>
#include <Methane/Graphics/RHI/CommandKit.h>
#include <Methane/Graphics/RHI/Program.h>
#include <Methane/Graphics/Types.h>
#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Data/Math.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace hlslpp // NOSONAR
{
#pragma pack(push, 16)
#include <TextUniforms.h> // NOSONAR
#pragma pack(pop)
}

#include <cassert>

namespace Methane::UserInterface
{

Text::Text(Context& ui_context, const Font& font, const SettingsUtf8&  settings)
    : Text(ui_context, ui_context.GetRenderPattern(), font, settings)
{
}

Text::Text(Context& ui_context, const rhi::RenderPattern& render_pattern, const Font& font, const SettingsUtf8& settings)
    : Text(ui_context, render_pattern, font,
        SettingsUtf32
        {
            settings.name,
            Font::ConvertUtf8To32(settings.text),
            settings.rect,
            settings.layout,
            settings.color,
            settings.incremental_update,
            settings.adjust_vertical_content_offset,
            settings.mesh_buffers_reservation_multiplier,
            settings.state_name
        }
    )
{
    META_FUNCTION_TASK();
}

Text::Text(Context& ui_context, const Font& font, SettingsUtf32 settings)
    : Text(ui_context, ui_context.GetRenderPattern(), font, std::move(settings))
{
}

Text::Text(Context& ui_context, const rhi::RenderPattern& render_pattern, const Font& font, SettingsUtf32 settings)
    : Item(ui_context, settings.rect)
    , m_settings(std::move(settings))
    , m_font(font)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_EMPTY_DESCR(m_settings.state_name, "Text state name can not be empty");

    m_font.Connect(*this);
    m_frame_rect = GetUIContext().ConvertTo<Units::Pixels>(m_settings.rect);

    SetRelOrigin(m_settings.rect.GetUnitOrigin());

    rhi::IObjectRegistry& gfx_objects_registry = ui_context.GetRenderContext().GetObjectRegistry();
    if (const auto render_state_ptr = std::dynamic_pointer_cast<rhi::IRenderState>(gfx_objects_registry.GetGraphicsObject(m_settings.state_name));
        render_state_ptr)
    {
        META_CHECK_ARG_EQUAL_DESCR(render_state_ptr->GetSettings().render_pattern_ptr->GetSettings(), render_pattern.GetSettings(),
                                   "Text '{}' render state '{}' from cache has incompatible render pattern settings", m_settings.name, m_settings.state_name);
        m_render_state = rhi::RenderState(render_state_ptr);
    }
    else
    {
        rhi::RenderState::Settings state_settings
        {
            rhi::Program(GetUIContext().GetRenderContext(),
            rhi::Program::Settings
            {
                rhi::Program::ShaderSet
                {
                    { rhi::ShaderType::Vertex, { Data::ShaderProvider::Get(), { "Text", "TextVS" }, { } } },
                    { rhi::ShaderType::Pixel,  { Data::ShaderProvider::Get(), { "Text", "TextPS" }, { } } },
                },
                rhi::ProgramInputBufferLayouts
                {
                    rhi::Program::InputBufferLayout
                    {
                        rhi::Program::InputBufferLayout::ArgumentSemantics { "POSITION", "TEXCOORD" }
                    }
                },
                rhi::ProgramArgumentAccessors
                {
                    { { rhi::ShaderType::Vertex, "g_uniforms"  }, rhi::ProgramArgumentAccessor::Type::Mutable  },
                    { { rhi::ShaderType::Pixel,  "g_constants" }, rhi::ProgramArgumentAccessor::Type::Mutable  },
                    { { rhi::ShaderType::Pixel,  "g_texture"   }, rhi::ProgramArgumentAccessor::Type::Mutable  },
                    { { rhi::ShaderType::Pixel,  "g_sampler"   }, rhi::ProgramArgumentAccessor::Type::Constant },
                },
                render_pattern.GetAttachmentFormats()
            }),
            render_pattern
        };
        state_settings.program.SetName("Text Shading");
        state_settings.depth.enabled                                        = false;
        state_settings.depth.write_enabled                                  = false;
        state_settings.rasterizer.is_front_counter_clockwise                = true;
        state_settings.blending.render_targets[0].blend_enabled             = true;
        state_settings.blending.render_targets[0].source_rgb_blend_factor   = rhi::IRenderState::Blending::Factor::SourceAlpha;
        state_settings.blending.render_targets[0].dest_rgb_blend_factor     = rhi::IRenderState::Blending::Factor::OneMinusSourceAlpha;
        state_settings.blending.render_targets[0].source_alpha_blend_factor = rhi::IRenderState::Blending::Factor::Zero;
        state_settings.blending.render_targets[0].dest_alpha_blend_factor   = rhi::IRenderState::Blending::Factor::Zero;

        m_render_state = GetUIContext().GetRenderContext().CreateRenderState(state_settings);
        m_render_state.SetName(m_settings.state_name);

        gfx_objects_registry.AddGraphicsObject(m_render_state.GetInterface());
    }

    UpdateTextMesh();

    const FrameRect viewport_rect = m_text_mesh_ptr ? GetAlignedViewportRect() : m_frame_rect.AsBase();
    m_view_state = rhi::ViewState({
        { gfx::GetFrameViewport(viewport_rect)    },
        { gfx::GetFrameScissorRect(viewport_rect) }
    });

    static const std::string s_sampler_name = "Font Atlas Sampler";
    if (const auto atlas_sampler_ptr = std::dynamic_pointer_cast<rhi::ISampler>(gfx_objects_registry.GetGraphicsObject(s_sampler_name));
        atlas_sampler_ptr)
    {
        m_atlas_sampler = rhi::Sampler(atlas_sampler_ptr);
    }
    else
    {
        m_atlas_sampler = GetUIContext().GetRenderContext().CreateSampler({
            rhi::ISampler::Filter(rhi::ISampler::Filter::MinMag::Linear),
            rhi::ISampler::Address(rhi::ISampler::Address::Mode::ClampToZero),
        });
        m_atlas_sampler.SetName(s_sampler_name);

        gfx_objects_registry.AddGraphicsObject(m_atlas_sampler.GetInterface());
    }

    Item::SetRect(m_frame_rect);
}

Text::~Text()
{
    META_FUNCTION_TASK();

    // Manually disconnect font, so that if it will be released along with text,
    // the destroyed text won't receive font atlas update callback leading to access violation
    m_font.Disconnect(*this);
}

std::string Text::GetTextUtf8() const
{
    META_FUNCTION_TASK();
    return Font::ConvertUtf32To8(m_settings.text);
}

void Text::SetText(std::string_view text)
{
    META_FUNCTION_TASK();
    SetTextInScreenRect(text, m_settings.rect);
}

void Text::SetText(std::u32string_view text)
{
    META_FUNCTION_TASK();
    SetTextInScreenRect(text, m_settings.rect);
}

void Text::SetTextInScreenRect(std::string_view text, const UnitRect& ui_rect)
{
    META_FUNCTION_TASK();
    SetTextInScreenRect(Font::ConvertUtf8To32(text), ui_rect);
}

void Text::SetTextInScreenRect(std::u32string_view text, const UnitRect& ui_rect)
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

    if (FrameResources& frame_resources = GetCurrentFrameResources();
        !frame_resources.IsAtlasInitialized())
    {
        // If atlas texture was not initialized it has to be requested for current context first to be properly updated in future
        frame_resources.UpdateAtlasTexture(m_font.GetAtlasTexture(GetUIContext().GetRenderContext()));
    }

    m_is_viewport_dirty = true;
    Item::SetRect(GetUIContext().ConvertTo<Units::Pixels>(m_frame_rect));
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
    return Item::SetRect(GetUIContext().ConvertTo<Units::Pixels>(m_frame_rect));
}

Text::UpdateRectResult Text::UpdateRect(const UnitRect& ui_rect, bool reset_content_rect)
{
    META_FUNCTION_TASK();
    const UnitRect& ui_curr_rect_px  = GetRectInPixels();
    const UnitRect  ui_rect_in_units = GetUIContext().ConvertToUnits(ui_rect, m_settings.rect.GetUnits());
    const UnitRect  ui_rect_in_px    = GetUIContext().ConvertTo<Units::Pixels>(ui_rect);
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

void Text::SetColor(const gfx::Color4F& color)
{
    META_FUNCTION_TASK();
    if (m_settings.color == color)
        return;

    m_settings.color = color;
    m_is_const_buffer_dirty = true;
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
    if (m_is_const_buffer_dirty)
    {
        UpdateConstantsBuffer();
    }
    if (frame_resources.IsDirty(FrameResources::DirtyResource::Mesh) && m_text_mesh_ptr)
    {
        frame_resources.UpdateMeshBuffers(GetUIContext().GetRenderContext(), *m_text_mesh_ptr, m_settings.name, m_settings.mesh_buffers_reservation_multiplier);
    }
    if (frame_resources.IsDirty(FrameResources::DirtyResource::Atlas))
    {
        frame_resources.UpdateAtlasTexture(m_font.GetAtlasTexture(GetUIContext().GetRenderContext()));
    }
    if (frame_resources.IsDirty(FrameResources::DirtyResource::Uniforms) && m_text_mesh_ptr)
    {
        frame_resources.UpdateUniformsBuffer(GetUIContext().GetRenderContext(), *m_text_mesh_ptr, m_settings.name);
    }
    if (m_render_state.IsInitialized())
    {
        frame_resources.InitializeProgramBindings(m_render_state, m_const_buffer, m_atlas_sampler, m_settings.name);
    }
    assert(!frame_resources.IsDirty() || !m_text_mesh_ptr);
}

void Text::Draw(const rhi::RenderCommandList& cmd_list, const rhi::CommandListDebugGroup* debug_group_ptr)
{
    META_FUNCTION_TASK();
    if (m_frame_resources.empty())
        return;

    const FrameResources& frame_resources = GetCurrentFrameResources();
    if (!frame_resources.IsInitialized())
        return;

    cmd_list.ResetWithStateOnce(m_render_state, debug_group_ptr);
    cmd_list.SetViewState(m_view_state);
    cmd_list.SetProgramBindings(frame_resources.GetProgramBindings());
    cmd_list.SetVertexBuffers(frame_resources.GetVertexBufferSet());
    cmd_list.SetIndexBuffer(frame_resources.GetIndexBuffer());
    cmd_list.DrawIndexed(rhi::RenderPrimitive::Triangle);
}

void Text::OnFontAtlasTextureReset(Font& font, const rhi::Texture* old_atlas_texture_ptr, const rhi::Texture* new_atlas_texture_ptr)
{
    META_FUNCTION_TASK();
    META_UNUSED(old_atlas_texture_ptr);
    if (std::addressof(m_font) != std::addressof(font) || m_frame_resources.empty() ||
        (new_atlas_texture_ptr && GetUIContext().GetRenderContext() != new_atlas_texture_ptr->GetRenderContext()))
        return;

    MakeFrameResourcesDirty(FrameResources::DirtyResourceMask(FrameResources::DirtyResource::Atlas));

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

Text::FrameResources::FrameResources(uint32_t frame_index, const CommonResourceRefs& common_resources)
    : m_frame_index(frame_index)
    , m_atlas_texture(common_resources.atlas_texture)
{
    META_FUNCTION_TASK();
}

void Text::FrameResources::SetDirty(DirtyResourceMask dirty_mask) noexcept
{
    META_FUNCTION_TASK();
    m_dirty_mask |= dirty_mask;
}

bool Text::FrameResources::IsDirty(DirtyResource resource) const noexcept
{
    META_FUNCTION_TASK();
    return m_dirty_mask.HasAnyBit(resource);
}

bool Text::FrameResources::IsDirty() const noexcept
{
    META_FUNCTION_TASK();
    return m_dirty_mask.HasAnyBits({
        DirtyResource::Mesh,
        DirtyResource::Uniforms,
        DirtyResource::Atlas
    });
}

void Text::FrameResources::InitializeProgramBindings(const rhi::RenderState& state, const rhi::Buffer& const_buffer,
                                                     const rhi::Sampler& atlas_sampler, std::string_view text_name)
{
    META_FUNCTION_TASK();
    if (m_program_bindings.IsInitialized())
        return;

    META_CHECK_ARG_TRUE(const_buffer.IsInitialized());
    META_CHECK_ARG_TRUE(atlas_sampler.IsInitialized());
    META_CHECK_ARG_TRUE(m_atlas_texture.IsInitialized());
    META_CHECK_ARG_TRUE(m_uniforms_buffer.IsInitialized());

    m_program_bindings = state.GetProgram().CreateBindings({
        { { rhi::ShaderType::Vertex, "g_uniforms"  }, { { m_uniforms_buffer.GetInterface() } } },
        { { rhi::ShaderType::Pixel,  "g_constants" }, { { const_buffer.GetInterface()      } } },
        { { rhi::ShaderType::Pixel,  "g_texture"   }, { { m_atlas_texture.GetInterface()   } } },
        { { rhi::ShaderType::Pixel,  "g_sampler"   }, { { atlas_sampler.GetInterface()     } } },
    });
    m_program_bindings.SetName(fmt::format("{} Text Bindings {}", text_name, m_frame_index));
}

bool Text::FrameResources::IsInitialized() const noexcept
{
    return m_program_bindings.IsInitialized() &&
           m_vertex_buffer_set.IsInitialized() &&
           m_index_buffer.IsInitialized();
}

bool Text::FrameResources::IsAtlasInitialized() const noexcept
{
    return !!m_atlas_texture.IsInitialized();
}

const rhi::BufferSet& Text::FrameResources::GetVertexBufferSet() const noexcept
{
    META_FUNCTION_TASK();
    return m_vertex_buffer_set;
}

const rhi::Buffer& Text::FrameResources::GetIndexBuffer() const noexcept
{
    return m_index_buffer;
}

const rhi::ProgramBindings& Text::FrameResources::GetProgramBindings() const noexcept
{
    return m_program_bindings;
}

bool Text::FrameResources::UpdateAtlasTexture(const rhi::Texture& new_atlas_texture)
{
    META_FUNCTION_TASK();
    m_dirty_mask.SetBitOff(DirtyResource::Atlas);

    if (m_atlas_texture == new_atlas_texture)
        return true;

    m_atlas_texture = new_atlas_texture;

    if (!m_atlas_texture.IsInitialized())
    {
        m_program_bindings = {};
        return true;
    }

    if (!m_program_bindings.IsInitialized())
        return false;

    m_program_bindings.Get({ rhi::ShaderType::Pixel, "g_texture" }).SetResourceViews({ { m_atlas_texture.GetInterface() } });
    return true;
}

void Text::FrameResources::UpdateMeshBuffers(const rhi::RenderContext& render_context, const TextMesh& text_mesh,
                                             std::string_view text_name, Data::Size reservation_multiplier)
{
    META_FUNCTION_TASK();

    // Update vertex buffer
    const Data::Size vertices_data_size = text_mesh.GetVerticesDataSize();
    META_CHECK_ARG_NOT_ZERO(vertices_data_size);
    
    if (!m_vertex_buffer_set.IsInitialized() || m_vertex_buffer_set[0].GetDataSize() < vertices_data_size)
    {
        const Data::Size  vertex_buffer_size = vertices_data_size * reservation_multiplier;
        rhi::Buffer vertex_buffer;
        vertex_buffer = render_context.CreateBuffer(rhi::BufferSettings::ForVertexBuffer(vertex_buffer_size, text_mesh.GetVertexSize()));
        vertex_buffer.SetName(fmt::format("{} Text Vertex Buffer {}", text_name, m_frame_index));
        m_vertex_buffer_set = rhi::BufferSet(rhi::BufferType::Vertex, { vertex_buffer });
    }
    m_vertex_buffer_set[0].SetData({
        rhi::SubResource(
            reinterpret_cast<Data::ConstRawPtr>(text_mesh.GetVertices().data()), vertices_data_size, // NOSONAR
            rhi::SubResource::Index(), rhi::BytesRange(0U, vertices_data_size)
        )
    }, render_context.GetRenderCommandKit().GetQueue());

    // Update index buffer
    const Data::Size indices_data_size = text_mesh.GetIndicesDataSize();
    META_CHECK_ARG_NOT_ZERO(indices_data_size);

    if (!m_index_buffer.IsInitialized() || m_index_buffer.GetDataSize() < indices_data_size)
    {
        const Data::Size index_buffer_size = vertices_data_size * reservation_multiplier;
        m_index_buffer = render_context.CreateBuffer(rhi::BufferSettings::ForIndexBuffer(index_buffer_size, gfx::PixelFormat::R16Uint));
        m_index_buffer.SetName(fmt::format("{} Text Index Buffer {}", text_name, m_frame_index));
    }

    m_index_buffer.SetData({
        rhi::SubResource(
            reinterpret_cast<Data::ConstRawPtr>(text_mesh.GetIndices().data()), indices_data_size, // NOSONAR
            rhi::SubResource::Index(), rhi::BytesRange(0U, indices_data_size)
        )
    }, render_context.GetRenderCommandKit().GetQueue());

    m_dirty_mask.SetBitOff(DirtyResource::Mesh);
}

void Text::FrameResources::UpdateUniformsBuffer(const rhi::RenderContext& render_context, const TextMesh& text_mesh, std::string_view text_name)
{
    META_FUNCTION_TASK();

    const gfx::FrameSize& content_size = text_mesh.GetContentSize();
    META_CHECK_ARG_NOT_ZERO_DESCR(content_size, "text uniforms buffer can not be updated when one of content size dimensions is zero");

    hlslpp::TextUniforms uniforms{
        hlslpp::mul(
            hlslpp::float4x4::scale(2.F / static_cast<float>(content_size.GetWidth()),
                                    2.F / static_cast<float>(content_size.GetHeight()),
                                    1.F),
            hlslpp::float4x4::translation(-1.F, 1.F, 0.F))
    };

    const auto uniforms_data_size = static_cast<Data::Size>(sizeof(uniforms));

    if (!m_uniforms_buffer.IsInitialized())
    {
        m_uniforms_buffer = render_context.CreateBuffer(rhi::BufferSettings::ForConstantBuffer(uniforms_data_size));
        m_uniforms_buffer.SetName(fmt::format("{} Text Uniforms Buffer {}", text_name, m_frame_index));

        if (m_program_bindings.IsInitialized())
        {
            m_program_bindings.Get({ rhi::ShaderType::Vertex, "g_uniforms" }).SetResourceViews({ { m_uniforms_buffer.GetInterface() } });
        }
    }
    m_uniforms_buffer.SetData(
        rhi::SubResources
        {
            { reinterpret_cast<Data::ConstRawPtr>(&uniforms), uniforms_data_size } // NOSONAR
        },
        render_context.GetRenderCommandKit().GetQueue()
    );

    m_dirty_mask.SetBitOff(DirtyResource::Uniforms);
}

void Text::InitializeFrameResources()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NAME_DESCR("m_frame_resources", m_frame_resources.empty(), "frame resources have been initialized already");
    META_CHECK_ARG_TRUE_DESCR(m_render_state.IsInitialized(), "text render state is not initialized");
    META_CHECK_ARG_NOT_NULL_DESCR(m_text_mesh_ptr, "text mesh is not initialized");

    const rhi::RenderContext& render_context = GetUIContext().GetRenderContext();
    const uint32_t frame_buffers_count = render_context.GetSettings().frame_buffers_count;
    m_frame_resources.reserve(frame_buffers_count);

    if (!m_const_buffer.IsInitialized())
    {
        m_const_buffer = render_context.CreateBuffer(rhi::BufferSettings::ForConstantBuffer(static_cast<Data::Size>(sizeof(hlslpp::TextConstants))));
        m_const_buffer.SetName(fmt::format("{} Text Constants Buffer", m_settings.name));
    }

    const rhi::Texture& atlas_texture = m_font.GetAtlasTexture(render_context);
    for(uint32_t frame_buffer_index = 0U; frame_buffer_index < frame_buffers_count; ++frame_buffer_index)
    {
        m_frame_resources.emplace_back(
            frame_buffer_index,
            CommonResourceRefs
            {
                render_context,
                m_render_state,
                m_const_buffer,
                atlas_texture,
                m_atlas_sampler,
                *m_text_mesh_ptr
            }
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

void Text::MakeFrameResourcesDirty(FrameResources::DirtyResourceMask resource)
{
    META_FUNCTION_TASK();
    for(FrameResources& frame_resources : m_frame_resources)
    {
        frame_resources.SetDirty(resource);
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

    // Fill font with new text chars strictly before building the text mesh, to be sure that font atlas size is up-to-date
    m_font.AddChars(m_settings.text);

    if (!m_font.GetAtlasSize())
        return;

    if (m_settings.incremental_update && m_text_mesh_ptr &&
        m_text_mesh_ptr->IsUpdatable(m_settings.text, m_settings.layout, m_font, m_frame_rect.size))
    {
        m_text_mesh_ptr->Update(m_settings.text, m_frame_rect.size);
    }
    else
    {
        m_text_mesh_ptr = std::make_unique<TextMesh>(m_settings.text, m_settings.layout, m_font, m_frame_rect.size);
    }

    if (m_frame_resources.empty() && m_render_state.IsInitialized())
    {
        InitializeFrameResources();
        return;
    }

    MakeFrameResourcesDirty(FrameResources::DirtyResourceMask({
        FrameResources::DirtyResource::Mesh,
        FrameResources::DirtyResource::Uniforms
    }));
}

void Text::UpdateConstantsBuffer()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_TRUE(m_const_buffer.IsInitialized());

    const hlslpp::TextConstants constants{
        m_settings.color.AsVector()
    };
    m_const_buffer.SetData(
        rhi::IResource::SubResources
        {
            rhi::IResource::SubResource(reinterpret_cast<Data::ConstRawPtr>(&constants), // NOSONAR
                                       static_cast<Data::Size>(sizeof(constants)))
        },
        GetUIContext().GetRenderContext().GetRenderCommandKit().GetQueue()
    );
    m_is_const_buffer_dirty = false;
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
        META_CHECK_ARG_LESS(content_top_offset, content_size.GetHeight() + 1);

        content_size.SetHeight(content_size.GetHeight() - content_top_offset);
        viewport_rect.origin.SetY(m_frame_rect.origin.GetY() - content_top_offset);
    }

    if (content_size.GetWidth() != m_frame_rect.size.GetWidth())
    {
        switch (m_settings.layout.horizontal_alignment)
        {
        case HorizontalAlignment::Justify:
        case HorizontalAlignment::Left:   break;
        case HorizontalAlignment::Right:  viewport_rect.origin.SetX(viewport_rect.origin.GetX() + static_cast<int32_t>(m_frame_rect.size.GetWidth() - content_size.GetWidth())); break;
        case HorizontalAlignment::Center: viewport_rect.origin.SetX(viewport_rect.origin.GetX() + static_cast<int32_t>(m_frame_rect.size.GetWidth() - content_size.GetWidth()) / 2); break;
        default:                          META_UNEXPECTED_ARG(m_settings.layout.horizontal_alignment);
        }
    }
    if (content_size.GetHeight() != m_frame_rect.size.GetHeight())
    {
        switch (m_settings.layout.vertical_alignment)
        {
        case VerticalAlignment::Top:      break;
        case VerticalAlignment::Bottom:   viewport_rect.origin.SetY(viewport_rect.origin.GetY() + static_cast<int32_t>(m_frame_rect.size.GetHeight() - content_size.GetHeight())); break;
        case VerticalAlignment::Center:   viewport_rect.origin.SetY(viewport_rect.origin.GetY() + static_cast<int32_t>(m_frame_rect.size.GetHeight() - content_size.GetHeight()) / 2); break;
        default:                          META_UNEXPECTED_ARG(m_settings.layout.vertical_alignment);
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
    m_view_state.SetViewports({ gfx::GetFrameViewport(viewport_rect) });
    m_view_state.SetScissorRects({ gfx::GetFrameScissorRect(viewport_rect, render_attachment_size) });
    m_is_viewport_dirty = false;
}

} // namespace Methane::Graphics
