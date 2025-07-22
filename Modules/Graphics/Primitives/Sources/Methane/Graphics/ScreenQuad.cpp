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

#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/RenderContext.h>
#include <Methane/Graphics/RHI/RenderCommandList.h>
#include <Methane/Graphics/RHI/RenderPass.h>
#include <Methane/Graphics/RHI/RenderState.h>
#include <Methane/Graphics/RHI/ViewState.h>
#include <Methane/Graphics/RHI/Buffer.h>
#include <Methane/Graphics/RHI/BufferSet.h>
#include <Methane/Graphics/RHI/Texture.h>
#include <Methane/Graphics/RHI/Sampler.h>
#include <Methane/Graphics/RHI/ProgramBindings.h>
#include <Methane/Graphics/RHI/ObjectRegistry.h>
#include <Methane/Graphics/QuadMesh.hpp>
#include <Methane/Graphics/TypeConverters.hpp>
#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>
#include <Methane/Pimpl.hpp>

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
        quad_name_ss << " " << Rhi::ShaderMacroDefinition::ToString(macro_definitions);
    return quad_name_ss.str();
}

class ScreenQuad::Impl
{
private:
    Settings                 m_settings;
    const Rhi::CommandQueue  m_render_cmd_queue;
    const Rhi::RenderPattern m_render_pattern;
    Rhi::RenderState         m_render_state;
    Rhi::ViewState           m_view_state;
    Rhi::BufferSet           m_vertex_buffer_set;
    Rhi::Buffer              m_index_buffer;
    Rhi::Texture             m_texture;
    Rhi::Sampler             m_texture_sampler;
    Rhi::ProgramBindings     m_const_program_bindings;

public:
    Impl(const Rhi::CommandQueue& render_cmd_queue, const Rhi::RenderPattern& render_pattern, const Settings& settings)
        : Impl(render_cmd_queue, render_pattern, Rhi::Texture(), settings)
    {
    }

    Impl(const Rhi::CommandQueue& render_cmd_queue, const Rhi::RenderPattern& render_pattern, const Rhi::Texture& texture, const Settings& settings)
        : m_settings(settings)
        , m_render_cmd_queue(render_cmd_queue)
        , m_render_pattern(render_pattern)
        , m_texture(texture)
    {
        META_FUNCTION_TASK();

        if (m_settings.texture_mode != TextureMode::Disabled)
        {
            META_CHECK_TRUE_DESCR(m_texture.IsInitialized(), "screen-quad texture can not be empty when quad texturing is enabled");
        }

        const Rhi::RenderContext render_context = render_pattern.GetRenderContext();
        static const QuadMesh<ScreenQuadVertex> s_quad_mesh(ScreenQuadVertex::layout, 2.F, 2.F);
        const Rhi::ShaderMacroDefinitions ps_macro_definitions = GetPixelShaderMacroDefinitions(m_settings.texture_mode);

        const std::string quad_name = GetQuadName(m_settings, ps_macro_definitions);
        const std::string state_name = fmt::format("{} Render State", quad_name);

        m_render_state = render_context.GetObjectRegistry().GetGraphicsObject<Rhi::RenderState>(state_name);
        if (!m_render_state.IsInitialized())
        {
            Rhi::RenderState::Settings state_settings
            {
                .program = Rhi::Program(render_context,
                    Rhi::Program::Settings
                    {
                        .shader_set = Rhi::Program::ShaderSet
                        {
                            { Rhi::ShaderType::Vertex, { Data::ShaderProvider::Get(), { "ScreenQuad", "QuadVS" }, { } } },
                            { Rhi::ShaderType::Pixel,  { Data::ShaderProvider::Get(), { "ScreenQuad", "QuadPS" }, ps_macro_definitions } },
                        },
                        .input_buffer_layouts = Rhi::ProgramInputBufferLayouts
                        {
                            Rhi::IProgram::InputBufferLayout
                            {
                                Rhi::IProgram::InputBufferLayout::ArgumentSemantics { s_quad_mesh.GetVertexLayout().GetSemantics() }
                            }
                        },
                        .argument_accessors = Rhi::ProgramArgumentAccessors
                        {
                            META_PROGRAM_ARG_ROOT_BUFFER_MUTABLE(Rhi::ShaderType::Pixel, "g_constants")
                        },
                        .attachment_formats = render_pattern.GetAttachmentFormats(),
                    }),
                .render_pattern = render_pattern,
                .rasterizer = Rhi::RasterizerSettings
                {
                    .is_front_counter_clockwise = true
                },
                .depth = Rhi::DepthSettings
                {
                    .enabled       = false,
                    .write_enabled = false
                }
            };
            state_settings.blending.render_targets[0] = Rhi::RenderTargetSettings
            {
                .blend_enabled             = m_settings.alpha_blending_enabled,
                .source_rgb_blend_factor   = Rhi::BlendingFactor::SourceAlpha,
                .source_alpha_blend_factor = Rhi::BlendingFactor::Zero,
                .dest_rgb_blend_factor     = Rhi::BlendingFactor::OneMinusSourceAlpha,
                .dest_alpha_blend_factor   = Rhi::BlendingFactor::Zero
            };
            state_settings.program.SetName(fmt::format("{} Shading", quad_name));

            m_render_state = render_context.CreateRenderState(state_settings);
            m_render_state.SetName(state_name);

            render_context.GetObjectRegistry().AddGraphicsObject(m_render_state);
        }

        m_view_state = Rhi::ViewState({
            { GetFrameViewport(m_settings.screen_rect)    },
            { GetFrameScissorRect(m_settings.screen_rect) }
        });

        if (m_settings.texture_mode != TextureMode::Disabled)
        {
            static const std::string s_sampler_name = "Screen-Quad Sampler";
            m_texture_sampler = render_context.GetObjectRegistry().GetGraphicsObject<Rhi::Sampler>(s_sampler_name);
            if (!m_texture_sampler.IsInitialized())
            {
                m_texture_sampler = render_context.CreateSampler(
                    Rhi::SamplerSettings
                    {
                        .filter  = Rhi::ISampler::Filter(Rhi::ISampler::Filter::MinMag::Linear),
                        .address = Rhi::ISampler::Address(Rhi::ISampler::Address::Mode::ClampToZero),
                    });
                m_texture_sampler.SetName(s_sampler_name);
                render_context.GetObjectRegistry().AddGraphicsObject(m_texture_sampler);
            }

            m_texture.SetName(fmt::format("{} Screen-Quad Texture", m_settings.name));
        }

        static const std::string s_vertex_buffer_set_name = "Screen-Quad Vertex Buffer Set";
        m_vertex_buffer_set = render_context.GetObjectRegistry().GetGraphicsObject<Rhi::BufferSet>(s_vertex_buffer_set_name);
        if (!m_vertex_buffer_set.IsInitialized())
        {
            Rhi::Buffer vertex_buffer;
            vertex_buffer = render_context.CreateBuffer(
                Rhi::BufferSettings::ForVertexBuffer(
                    s_quad_mesh.GetVertexDataSize(),
                    s_quad_mesh.GetVertexSize()));
            vertex_buffer.SetName("Screen-Quad Vertex Buffer");
            vertex_buffer.SetData(m_render_cmd_queue, {
                reinterpret_cast<Data::ConstRawPtr>(s_quad_mesh.GetVertices().data()), // NOSONAR
                s_quad_mesh.GetVertexDataSize()
            });
            m_vertex_buffer_set = Rhi::BufferSet(Rhi::BufferType::Vertex, { vertex_buffer });
            m_vertex_buffer_set.SetName(s_vertex_buffer_set_name);
            render_context.GetObjectRegistry().AddGraphicsObject(m_vertex_buffer_set);
        }

        static const std::string s_index_buffer_name = "Screen-Quad Index Buffer";
        m_index_buffer = render_context.GetObjectRegistry().GetGraphicsObject<Rhi::Buffer>(s_index_buffer_name);
        if (!m_index_buffer.IsInitialized())
        {
            m_index_buffer = render_context.CreateBuffer(
                Rhi::BufferSettings::ForIndexBuffer(
                    s_quad_mesh.GetIndexDataSize(),
                    GetIndexFormat(s_quad_mesh.GetIndex(0))));
            m_index_buffer.SetName(s_index_buffer_name);
            m_index_buffer.SetData(m_render_cmd_queue, {
                reinterpret_cast<Data::ConstRawPtr>(s_quad_mesh.GetIndices().data()), // NOSONAR
                s_quad_mesh.GetIndexDataSize()
            });
            render_context.GetObjectRegistry().AddGraphicsObject(m_index_buffer);
        }

        Rhi::ProgramBindings::BindingValueByArgument program_binding_resource_views;
        if (m_settings.texture_mode != TextureMode::Disabled)
        {
            program_binding_resource_views = {
                { { Rhi::ShaderType::Pixel, "g_texture" }, m_texture.GetResourceView() },
                { { Rhi::ShaderType::Pixel, "g_sampler" }, m_texture_sampler.GetResourceView() }
            };
        }

        m_const_program_bindings = m_render_state.GetProgram().CreateBindings(program_binding_resource_views);
        m_const_program_bindings.SetName(fmt::format("{} Screen-Quad Constant Bindings", m_settings.name));

        UpdateConstants();
    }

    void SetBlendColor(const Color4F& blend_color)
    {
        META_FUNCTION_TASK();
        if (m_settings.blend_color == blend_color)
            return;

        m_settings.blend_color  = blend_color;
        UpdateConstants();
    }

    void SetScreenRect(const FrameRect& screen_rect, const FrameSize& render_attachment_size)
    {
        META_FUNCTION_TASK();
        if (m_settings.screen_rect == screen_rect)
            return;

        m_settings.screen_rect = screen_rect;

        m_view_state.SetViewports({ GetFrameViewport(screen_rect) });
        m_view_state.SetScissorRects({ GetFrameScissorRect(screen_rect, render_attachment_size) });
    }

    void SetAlphaBlendingEnabled(bool alpha_blending_enabled)
    {
        META_FUNCTION_TASK();
        if (m_settings.alpha_blending_enabled == alpha_blending_enabled)
            return;

        m_settings.alpha_blending_enabled = alpha_blending_enabled;

        Rhi::IRenderState::Settings state_settings = m_render_state.GetSettings();
        state_settings.blending.render_targets[0].blend_enabled = alpha_blending_enabled;
        m_render_state.Reset(state_settings);
    }

    void SetTexture(Rhi::Texture texture)
    {
        META_FUNCTION_TASK();
        META_CHECK_NOT_EQUAL_DESCR(m_settings.texture_mode, TextureMode::Disabled, "can not set texture of screen quad with Disabled texture mode");
        META_CHECK_TRUE_DESCR(texture.IsInitialized(), "can not set null texture to screen quad");

        if (m_texture == texture)
            return;

        m_texture = texture;
        m_const_program_bindings.Get({ Rhi::ShaderType::Pixel, "g_texture" })
                                .SetResourceView(m_texture.GetResourceView());
    }

    [[nodiscard]] const Settings& GetQuadSettings() const noexcept
    {
        return m_settings;
    }

    [[nodiscard]] const Rhi::Texture& GetTexture() const noexcept
    {
        return m_texture;
    }

    void Draw(const Rhi::RenderCommandList& cmd_list, const Rhi::CommandListDebugGroup* debug_group_ptr = nullptr) const
    {
        META_FUNCTION_TASK();
        cmd_list.ResetWithStateOnce(m_render_state, debug_group_ptr);
        cmd_list.SetViewState(m_view_state);
        cmd_list.SetProgramBindings(m_const_program_bindings);
        cmd_list.SetVertexBuffers(m_vertex_buffer_set);
        cmd_list.SetIndexBuffer(m_index_buffer);
        cmd_list.DrawIndexed(Rhi::RenderPrimitive::Triangle);
    }

private:
    void UpdateConstants() const
    {
        META_FUNCTION_TASK();
        const hlslpp::ScreenQuadConstants constants {
            m_settings.blend_color.AsVector()
        };

        m_const_program_bindings.Get({ Rhi::ShaderType::Pixel, "g_constants" })
                                .SetRootConstant(Rhi::RootConstant(constants));
    }

    [[nodiscard]] static Rhi::IShader::MacroDefinitions GetPixelShaderMacroDefinitions(TextureMode texture_mode)
    {
        META_FUNCTION_TASK();
        Rhi::IShader::MacroDefinitions macro_definitions;

        switch(texture_mode)
        {
        using enum TextureMode;
        case Disabled:
            macro_definitions.emplace_back("TEXTURE_DISABLED", "");
            break;

        case RgbaFloat:
            break;

        case RFloatToAlpha:
            macro_definitions.emplace_back("TTEXEL", "float");
            macro_definitions.emplace_back("RMASK", "r");
            macro_definitions.emplace_back("WMASK", "a");
            break;

        default:
            META_UNEXPECTED(texture_mode);
        }

        return macro_definitions;
    }
};

ScreenQuad::ScreenQuad(const Rhi::CommandQueue& render_cmd_queue, const Rhi::RenderPattern& render_pattern, const Settings& settings)
    : m_impl_ptr(std::make_shared<Impl>(render_cmd_queue, render_pattern, settings))
{
}

ScreenQuad::ScreenQuad(const Rhi::CommandQueue& render_cmd_queue, const Rhi::RenderPattern& render_pattern,
                       const Rhi::Texture& texture, const Settings& settings)
    : m_impl_ptr(std::make_shared<Impl>(render_cmd_queue, render_pattern, texture, settings))
{
}

void ScreenQuad::SetBlendColor(const Color4F& blend_color) const
{
    GetImpl(m_impl_ptr).SetBlendColor(blend_color);
}

void ScreenQuad::SetScreenRect(const FrameRect& screen_rect, const FrameSize& render_attachment_size) const
{
    GetImpl(m_impl_ptr).SetScreenRect(screen_rect, render_attachment_size);
}

void ScreenQuad::SetAlphaBlendingEnabled(bool alpha_blending_enabled) const
{
    GetImpl(m_impl_ptr).SetAlphaBlendingEnabled(alpha_blending_enabled);
}

void ScreenQuad::SetTexture(Rhi::Texture texture) const
{
    GetImpl(m_impl_ptr).SetTexture(texture);
}

[[nodiscard]] const ScreenQuad::Settings& ScreenQuad::GetQuadSettings() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetQuadSettings();
}

[[nodiscard]] const Rhi::Texture& ScreenQuad::GetTexture() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetTexture();
}

void ScreenQuad::Draw(const Rhi::RenderCommandList& cmd_list, const Rhi::CommandListDebugGroup* debug_group_ptr) const
{
    GetImpl(m_impl_ptr).Draw(cmd_list, debug_group_ptr);
}

} // namespace Methane::Graphics
