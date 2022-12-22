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

FILE: Methane/Graphics/ScreenQuad.cpp
Screen Quad rendering primitive.

******************************************************************************/

#include <Methane/Graphics/ScreenQuad.h>

#include <Methane/Graphics/RHI/IRenderContext.h>
#include <Methane/Graphics/RHI/ICommandQueue.h>
#include <Methane/Graphics/RHI/ICommandList.h>
#include <Methane/Graphics/RHI/ITexture.h>
#include <Methane/Graphics/RHI/IBuffer.h>
#include <Methane/Graphics/RHI/IRenderState.h>
#include <Methane/Graphics/RHI/IProgram.h>
#include <Methane/Graphics/RHI/IProgramBindings.h>
#include <Methane/Graphics/RHI/ISampler.h>
#include <Methane/Graphics/QuadMesh.hpp>
#include <Methane/Graphics/RHI/IRenderPass.h>
#include <Methane/Graphics/RHI/ICommandKit.h>
#include <Methane/Graphics/RHI/IRenderCommandList.h>
#include <Methane/Graphics/TypeConverters.hpp>
#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace hlslpp // NOSONAR
{
#pragma pack(push, 16)
#include <ScreenQuadConstants.h> // NOSONAR
#pragma pack(pop)
}

#include <fmt/format.h>

namespace Methane::Graphics
{

struct ScreenQuadVertex
{
    Mesh::Position position;
    Mesh::TexCoord texcoord;

    inline static const Mesh::VertexLayout layout {
        Mesh::VertexField::Position,
        Mesh::VertexField::TexCoord,
    };
};

static std::string GetQuadName(const ScreenQuad::Settings& settings, const Rhi::IShader::MacroDefinitions& macro_definitions)
{
    META_FUNCTION_TASK();
    std::stringstream quad_name_ss;
    quad_name_ss << "Screen-Quad";
    if (settings.alpha_blending_enabled)
        quad_name_ss << " with Alpha-Blending";
    if (!macro_definitions.empty())
        quad_name_ss << " " << Rhi::IShader::ConvertMacroDefinitionsToString(macro_definitions);
    return quad_name_ss.str();
}

ScreenQuad::ScreenQuad(Rhi::ICommandQueue& render_cmd_queue, Rhi::IRenderPattern& render_pattern, const Settings& settings)
    : ScreenQuad(render_cmd_queue, render_pattern, nullptr, settings)
{
}

ScreenQuad::ScreenQuad(Rhi::ICommandQueue& render_cmd_queue, Rhi::IRenderPattern& render_pattern, const Ptr<Rhi::ITexture>& texture_ptr, const Settings& settings)
    : m_settings(settings)
    , m_render_cmd_queue_ptr(render_cmd_queue.GetDerivedPtr<Rhi::ICommandQueue>())
    , m_render_pattern_ptr(render_pattern.GetDerivedPtr<Rhi::IRenderPattern>())
    , m_texture_ptr(texture_ptr)
{
    META_FUNCTION_TASK();
    if (m_settings.texture_mode != TextureMode::Disabled)
    {
        META_CHECK_ARG_NOT_NULL_DESCR(m_texture_ptr, "screen-quad texture can not be empty when quad texturing is enabled");
    }

    Rhi::IRenderContext& render_context = render_pattern.GetRenderContext();
    static const QuadMesh<ScreenQuadVertex> quad_mesh(ScreenQuadVertex::layout, 2.F, 2.F);
    const Rhi::IShader::MacroDefinitions ps_macro_definitions       = GetPixelShaderMacroDefinitions(m_settings.texture_mode);
    Rhi::ProgramArgumentAccessors program_argument_accessors = {
        { { Rhi::ShaderType::Pixel, "g_constants" }, Rhi::ProgramArgumentAccessType::Mutable }
    };

    if (m_settings.texture_mode != TextureMode::Disabled)
    {
        program_argument_accessors.emplace(Rhi::ShaderType::Pixel, "g_texture", Rhi::ProgramArgumentAccessType::Mutable);
        program_argument_accessors.emplace(Rhi::ShaderType::Pixel, "g_sampler", Rhi::ProgramArgumentAccessType::Constant);
    }

    const std::string quad_name = GetQuadName(m_settings, ps_macro_definitions);
    const std::string state_name = fmt::format("{} Render State", quad_name);
    m_render_state_ptr = std::dynamic_pointer_cast<Rhi::IRenderState>(render_context.GetObjectRegistry().GetGraphicsObject(state_name));
    if (!m_render_state_ptr)
    {
        Rhi::IRenderState::Settings state_settings;
        state_settings.program_ptr = Rhi::IProgram::Create(render_context,
            Rhi::IProgram::Settings
            {
                Rhi::IProgram::Shaders
                {
                    Rhi::IShader::CreateVertex(render_context, { Data::ShaderProvider::Get(), { "ScreenQuad", "QuadVS" }, { } }),
                    Rhi::IShader::CreatePixel(render_context, { Data::ShaderProvider::Get(), { "ScreenQuad", "QuadPS" }, ps_macro_definitions }),
                },
                Rhi::ProgramInputBufferLayouts
                {
                    Rhi::IProgram::InputBufferLayout
                    {
                        Rhi::IProgram::InputBufferLayout::ArgumentSemantics { quad_mesh.GetVertexLayout().GetSemantics() }
                    }
                },
                program_argument_accessors,
                render_pattern.GetAttachmentFormats(),
            }
        );
        state_settings.program_ptr->SetName(fmt::format("{} Shading", quad_name));
        state_settings.render_pattern_ptr                                   = render_pattern.GetDerivedPtr<Rhi::IRenderPattern>();
        state_settings.depth.enabled                                        = false;
        state_settings.depth.write_enabled                                  = false;
        state_settings.rasterizer.is_front_counter_clockwise                = true;
        state_settings.blending.render_targets[0].blend_enabled             = m_settings.alpha_blending_enabled;
        state_settings.blending.render_targets[0].source_rgb_blend_factor   = Rhi::IRenderState::Blending::Factor::SourceAlpha;
        state_settings.blending.render_targets[0].dest_rgb_blend_factor     = Rhi::IRenderState::Blending::Factor::OneMinusSourceAlpha;
        state_settings.blending.render_targets[0].source_alpha_blend_factor = Rhi::IRenderState::Blending::Factor::Zero;
        state_settings.blending.render_targets[0].dest_alpha_blend_factor   = Rhi::IRenderState::Blending::Factor::Zero;

        m_render_state_ptr = Rhi::IRenderState::Create(render_context, state_settings);
        m_render_state_ptr->SetName(state_name);

        render_context.GetObjectRegistry().AddGraphicsObject(*m_render_state_ptr);
    }

    m_view_state_ptr = Rhi::IViewState::Create({
        { GetFrameViewport(m_settings.screen_rect)    },
        { GetFrameScissorRect(m_settings.screen_rect) }
    });

    if (m_settings.texture_mode != TextureMode::Disabled)
    {
        static const std::string s_sampler_name = "Screen-Quad Sampler";
        m_texture_sampler_ptr = std::dynamic_pointer_cast<Rhi::ISampler>(render_context.GetObjectRegistry().GetGraphicsObject(s_sampler_name));
        if (!m_texture_sampler_ptr)
        {
            m_texture_sampler_ptr = Rhi::ISampler::Create(render_context, {
                Rhi::ISampler::Filter(Rhi::ISampler::Filter::MinMag::Linear),
                Rhi::ISampler::Address(Rhi::ISampler::Address::Mode::ClampToZero),
            });
            m_texture_sampler_ptr->SetName(s_sampler_name);
            render_context.GetObjectRegistry().AddGraphicsObject(*m_texture_sampler_ptr);
        }

        m_texture_ptr->SetName(fmt::format("{} Screen-Quad Texture", m_settings.name));
    }

    static const std::string s_vertex_buffer_name = "Screen-Quad Vertex Buffer";
    Ptr<Rhi::IBuffer> vertex_buffer_ptr = std::dynamic_pointer_cast<Rhi::IBuffer>(render_context.GetObjectRegistry().GetGraphicsObject(s_vertex_buffer_name));
    if (!vertex_buffer_ptr)
    {
        vertex_buffer_ptr = Rhi::IBuffer::CreateVertexBuffer(render_context, quad_mesh.GetVertexDataSize(), quad_mesh.GetVertexSize());
        vertex_buffer_ptr->SetName(s_vertex_buffer_name);
        vertex_buffer_ptr->SetData({
                {
                    reinterpret_cast<Data::ConstRawPtr>(quad_mesh.GetVertices().data()), // NOSONAR
                    quad_mesh.GetVertexDataSize()
                }
            },
            *m_render_cmd_queue_ptr);
        render_context.GetObjectRegistry().AddGraphicsObject(*vertex_buffer_ptr);
    }

    m_vertex_buffer_set_ptr = Rhi::IBufferSet::CreateVertexBuffers({ *vertex_buffer_ptr });

    static const std::string s_index_buffer_name = "Screen-Quad Index Buffer";
    m_index_buffer_ptr = std::dynamic_pointer_cast<Rhi::IBuffer>(render_context.GetObjectRegistry().GetGraphicsObject(s_index_buffer_name));
    if (!m_index_buffer_ptr)
    {
        m_index_buffer_ptr = Rhi::IBuffer::CreateIndexBuffer(render_context, quad_mesh.GetIndexDataSize(), GetIndexFormat(quad_mesh.GetIndex(0)));
        m_index_buffer_ptr->SetName(s_index_buffer_name);
        m_index_buffer_ptr->SetData({
                {
                    reinterpret_cast<Data::ConstRawPtr>(quad_mesh.GetIndices().data()), // NOSONAR
                    quad_mesh.GetIndexDataSize()
                }
            },
            *m_render_cmd_queue_ptr);
        render_context.GetObjectRegistry().AddGraphicsObject(*m_index_buffer_ptr);
    }

    m_const_buffer_ptr = Rhi::IBuffer::CreateConstantBuffer(render_context, static_cast<Data::Size>(sizeof(hlslpp::ScreenQuadConstants)));
    m_const_buffer_ptr->SetName(fmt::format("{} Screen-Quad Constants Buffer", m_settings.name));

    Rhi::IProgramBindings::ResourceViewsByArgument program_binding_resource_views = {
        { { Rhi::ShaderType::Pixel, "g_constants" }, { { *m_const_buffer_ptr    } } }
    };

    if (m_settings.texture_mode != TextureMode::Disabled)
    {
        program_binding_resource_views.try_emplace(Rhi::IProgram::Argument(Rhi::ShaderType::Pixel, "g_texture"), Rhi::IResource::Views{ { *m_texture_ptr         } });
        program_binding_resource_views.try_emplace(Rhi::IProgram::Argument(Rhi::ShaderType::Pixel, "g_sampler"), Rhi::IResource::Views{ { *m_texture_sampler_ptr } });
    }

    m_const_program_bindings_ptr = Rhi::IProgramBindings::Create(*m_render_state_ptr->GetSettings().program_ptr, program_binding_resource_views);
    m_const_program_bindings_ptr->SetName(fmt::format("{} Screen-Quad Constant Bindings", m_settings.name));

    UpdateConstantsBuffer();
}

void ScreenQuad::SetBlendColor(const Color4F& blend_color)
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

    m_view_state_ptr->SetViewports({ GetFrameViewport(screen_rect) });
    m_view_state_ptr->SetScissorRects({ GetFrameScissorRect(screen_rect, render_attachment_size) });
}

void ScreenQuad::SetAlphaBlendingEnabled(bool alpha_blending_enabled)
{
    META_FUNCTION_TASK();
    if (m_settings.alpha_blending_enabled == alpha_blending_enabled)
        return;

    m_settings.alpha_blending_enabled = alpha_blending_enabled;

    Rhi::IRenderState::Settings state_settings = m_render_state_ptr->GetSettings();
    state_settings.blending.render_targets[0].blend_enabled = alpha_blending_enabled;
    m_render_state_ptr->Reset(state_settings);
}

void ScreenQuad::SetTexture(Ptr<Rhi::ITexture> texture_ptr)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_EQUAL_DESCR(m_settings.texture_mode, TextureMode::Disabled, "can not set texture of screen quad with Disabled texture mode");
    META_CHECK_ARG_NOT_NULL_DESCR(texture_ptr, "can not set null texture to screen quad");

    if (m_texture_ptr.get() == texture_ptr.get())
        return;

    m_texture_ptr = texture_ptr;
    m_const_program_bindings_ptr->Get({ Rhi::ShaderType::Pixel, "g_texture" }).SetResourceViews({ { *m_texture_ptr } });
}

const Rhi::ITexture& ScreenQuad::GetTexture() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_texture_ptr);
    return *m_texture_ptr;
}

void ScreenQuad::Draw(Rhi::IRenderCommandList& cmd_list, Rhi::ICommandListDebugGroup* debug_group_ptr) const
{
    META_FUNCTION_TASK();
    cmd_list.ResetWithStateOnce(*m_render_state_ptr, debug_group_ptr);
    cmd_list.SetViewState(*m_view_state_ptr);
    cmd_list.SetProgramBindings(*m_const_program_bindings_ptr);
    cmd_list.SetVertexBuffers(*m_vertex_buffer_set_ptr);
    cmd_list.SetIndexBuffer(*m_index_buffer_ptr);
    cmd_list.DrawIndexed(Rhi::RenderPrimitive::Triangle);
}

const Rhi::IRenderContext& ScreenQuad::GetRenderContext() const noexcept
{
    META_FUNCTION_TASK();
    return GetRenderPattern().GetRenderContext();
}

void ScreenQuad::UpdateConstantsBuffer() const
{
    META_FUNCTION_TASK();
    const hlslpp::ScreenQuadConstants constants {
        m_settings.blend_color.AsVector()
    };

    m_const_buffer_ptr->SetData(
        Rhi::IResource::SubResources
        {
            {
                reinterpret_cast<Data::ConstRawPtr>(&constants), // NOSONAR
                static_cast<Data::Size>(sizeof(constants))
            }
        },
        *m_render_cmd_queue_ptr
    );
}

Rhi::IShader::MacroDefinitions ScreenQuad::GetPixelShaderMacroDefinitions(TextureMode texture_mode)
{
    META_FUNCTION_TASK();
    Rhi::IShader::MacroDefinitions macro_definitions;

    switch(texture_mode)
    {
    case TextureMode::Disabled:
        macro_definitions.emplace_back("TEXTURE_DISABLED", "");
        break;

    case TextureMode::RgbaFloat:
        break;

    case TextureMode::RFloatToAlpha:
        macro_definitions.emplace_back("TTEXEL", "float");
        macro_definitions.emplace_back("RMASK", "r");
        macro_definitions.emplace_back("WMASK", "a");
        break;

    default:
        META_UNEXPECTED_ARG(texture_mode);
    }

    return macro_definitions;
}

} // namespace Methane::Graphics
