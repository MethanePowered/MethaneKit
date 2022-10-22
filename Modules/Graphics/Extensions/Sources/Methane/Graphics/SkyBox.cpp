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

FILE: Methane/Graphics/SkyBox.cpp
SkyBox rendering primitive

******************************************************************************/

#include <Methane/Graphics/SkyBox.h>
#include <Methane/Graphics/CubeMesh.hpp>
#include <Methane/Graphics/Camera.h>
#include <Methane/Graphics/CommandQueue.h>
#include <Methane/Graphics/IRenderContext.h>
#include <Methane/Graphics/RenderPass.h>
#include <Methane/Graphics/RenderState.h>
#include <Methane/Graphics/Buffer.h>
#include <Methane/Graphics/Program.h>
#include <Methane/Graphics/Sampler.h>
#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Instrumentation.h>

#include <magic_enum.hpp>

namespace Methane::Graphics
{

SkyBox::SkyBox(CommandQueue& render_cmd_queue, RenderPattern& render_pattern, Texture& cube_map_texture, const Settings& settings)
    : SkyBox(render_cmd_queue, render_pattern, cube_map_texture, settings, CubeMesh<Vertex>(Vertex::layout))
{
}

SkyBox::SkyBox(CommandQueue& render_cmd_queue, RenderPattern& render_pattern, Texture& cube_map_texture,
               const Settings& settings, const BaseMesh<Vertex>& mesh)
    : m_settings(settings)
    , m_render_cmd_queue_ptr(std::dynamic_pointer_cast<CommandQueue>(render_cmd_queue.GetPtr()))
    , m_context(render_pattern.GetRenderContext())
    , m_mesh_buffers(render_cmd_queue, mesh, "Sky-Box")
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL(cube_map_texture.GetSettings().dimension_type, Texture::DimensionType::Cube);
    m_mesh_buffers.SetTexture(std::dynamic_pointer_cast<Texture>(cube_map_texture.GetPtr()));

    RenderState::Settings state_settings;
    state_settings.program_ptr = Program::Create(m_context,
        Program::Settings
        {
            Program::Shaders
            {
                Shader::CreateVertex(m_context, { Data::ShaderProvider::Get(), { "SkyBox", "SkyboxVS" }, { } }),
                Shader::CreatePixel( m_context, { Data::ShaderProvider::Get(), { "SkyBox", "SkyboxPS" }, { } }),
            },
            Program::InputBufferLayouts
            {
                Program::InputBufferLayout
                {
                    Program::InputBufferLayout::ArgumentSemantics { mesh.GetVertexLayout().GetSemantics() }
                }
            },
            Program::ArgumentAccessors
            {
                { { Shader::Type::Vertex, "g_skybox_uniforms" }, Program::ArgumentAccessor::Type::FrameConstant },
                { { Shader::Type::Pixel,  "g_skybox_texture"  }, Program::ArgumentAccessor::Type::Constant      },
                { { Shader::Type::Pixel,  "g_texture_sampler" }, Program::ArgumentAccessor::Type::Constant      },
            },
            render_pattern.GetAttachmentFormats()
        }
    );

    using namespace magic_enum::bitwise_operators;
    state_settings.program_ptr->SetName("Sky-box shading");
    state_settings.render_pattern_ptr   = std::dynamic_pointer_cast<RenderPattern>(render_pattern.GetPtr());
    state_settings.depth.enabled        = static_cast<bool>(m_settings.render_options & Options::DepthEnabled);
    state_settings.depth.write_enabled  = false;
    state_settings.depth.compare        = static_cast<bool>(m_settings.render_options & Options::DepthReversed) ? Compare::GreaterEqual : Compare::Less;
    state_settings.rasterizer.is_front_counter_clockwise = true;

    m_render_state_ptr = RenderState::Create(m_context, state_settings);
    m_render_state_ptr->SetName("Sky-box render state");

    m_texture_sampler_ptr = Sampler::Create(m_context, {
        Sampler::Filter(Sampler::Filter::MinMag::Linear),
        Sampler::Address(Sampler::Address::Mode::ClampToZero),
        Sampler::LevelOfDetail(m_settings.lod_bias)
    });
    m_texture_sampler_ptr->SetName("Sky-box Texture Sampler");
}

Ptr<ProgramBindings> SkyBox::CreateProgramBindings(const Ptr<Buffer>& uniforms_buffer_ptr, Data::Index frame_index) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(uniforms_buffer_ptr);
    META_CHECK_ARG_NOT_NULL(m_texture_sampler_ptr);
    META_CHECK_ARG_NOT_NULL(m_render_state_ptr);
    META_CHECK_ARG_NOT_NULL(m_render_state_ptr->GetSettings().program_ptr);
    return ProgramBindings::Create(m_render_state_ptr->GetSettings().program_ptr, {
        { { Shader::Type::Vertex, "g_skybox_uniforms" }, { { *uniforms_buffer_ptr         } } },
        { { Shader::Type::Pixel,  "g_skybox_texture"  }, { {  m_mesh_buffers.GetTexture() } } },
        { { Shader::Type::Pixel,  "g_texture_sampler" }, { { *m_texture_sampler_ptr       } } },
    }, frame_index);
}

void SkyBox::Update()
{
    META_FUNCTION_TASK();
    m_mesh_buffers.SetFinalPassUniforms({
        hlslpp::transpose(hlslpp::mul(
            hlslpp::mul(
                hlslpp::float4x4::scale(m_settings.scale),
                hlslpp::float4x4::translation(m_settings.view_camera.GetOrientation().eye)),
            m_settings.view_camera.GetViewProjMatrix()
        ))
    });
}

void SkyBox::Draw(RenderCommandList& cmd_list, const MeshBufferBindings& buffer_bindings, ViewState& view_state)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(buffer_bindings.program_bindings_ptr);
    META_CHECK_ARG_NOT_NULL(buffer_bindings.uniforms_buffer_ptr);
    META_CHECK_ARG_GREATER_OR_EQUAL(buffer_bindings.uniforms_buffer_ptr->GetDataSize(), sizeof(Uniforms));

    buffer_bindings.uniforms_buffer_ptr->SetData(m_mesh_buffers.GetFinalPassUniformsSubresources(), *m_render_cmd_queue_ptr);

    META_DEBUG_GROUP_CREATE_VAR(s_debug_group, "Sky-box rendering");
    cmd_list.ResetWithStateOnce(*m_render_state_ptr, s_debug_group.get());
    cmd_list.SetViewState(view_state);
    m_mesh_buffers.Draw(cmd_list, *buffer_bindings.program_bindings_ptr);
}

} // namespace Methane::Graphics
