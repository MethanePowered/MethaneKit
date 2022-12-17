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
#include <Methane/Graphics/RHI/ICommandQueue.h>
#include <Methane/Graphics/RHI/ICommandListDebugGroup.h>
#include <Methane/Graphics/RHI/IRenderContext.h>
#include <Methane/Graphics/RHI/IRenderPass.h>
#include <Methane/Graphics/RHI/IRenderState.h>
#include <Methane/Graphics/RHI/IBuffer.h>
#include <Methane/Graphics/RHI/IProgram.h>
#include <Methane/Graphics/RHI/ISampler.h>
#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

SkyBox::SkyBox(Rhi::ICommandQueue& render_cmd_queue, Rhi::IRenderPattern& render_pattern, Rhi::ITexture& cube_map_texture, const Settings& settings)
    : SkyBox(render_cmd_queue, render_pattern, cube_map_texture, settings, CubeMesh<Vertex>(Vertex::layout))
{
}

SkyBox::SkyBox(Rhi::ICommandQueue& render_cmd_queue, Rhi::IRenderPattern& render_pattern, Rhi::ITexture& cube_map_texture,
               const Settings& settings, const BaseMesh<Vertex>& mesh)
    : m_settings(settings)
    , m_render_cmd_queue_ptr(std::dynamic_pointer_cast<Rhi::ICommandQueue>(render_cmd_queue.GetPtr()))
    , m_context(render_pattern.GetRenderContext())
    , m_mesh_buffers(render_cmd_queue, mesh, "Sky-Box")
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL(cube_map_texture.GetSettings().dimension_type, Rhi::TextureDimensionType::Cube);
    m_mesh_buffers.SetTexture(std::dynamic_pointer_cast<Rhi::ITexture>(cube_map_texture.GetPtr()));

    Rhi::IRenderState::Settings state_settings;
    state_settings.program_ptr = Rhi::IProgram::Create(m_context,
        Rhi::IProgram::Settings
        {
            Rhi::IProgram::Shaders
            {
                Rhi::IShader::CreateVertex(m_context, { Data::ShaderProvider::Get(), { "SkyBox", "SkyboxVS" }, { } }),
                Rhi::IShader::CreatePixel( m_context, { Data::ShaderProvider::Get(), { "SkyBox", "SkyboxPS" }, { } }),
            },
            Rhi::ProgramInputBufferLayouts
            {
                Rhi::IProgram::InputBufferLayout
                {
                    Rhi::IProgram::InputBufferLayout::ArgumentSemantics { mesh.GetVertexLayout().GetSemantics() }
                }
            },
            Rhi::ProgramArgumentAccessors
            {
                { { Rhi::ShaderType::Vertex, "g_skybox_uniforms" }, Rhi::ProgramArgumentAccessType::FrameConstant },
                { { Rhi::ShaderType::Pixel,  "g_skybox_texture"  }, Rhi::ProgramArgumentAccessType::Constant      },
                { { Rhi::ShaderType::Pixel,  "g_texture_sampler" }, Rhi::ProgramArgumentAccessType::Constant      },
            },
            render_pattern.GetAttachmentFormats()
        }
    );

    state_settings.program_ptr->SetName("Sky-box shading");
    state_settings.render_pattern_ptr   = std::dynamic_pointer_cast<Rhi::IRenderPattern>(render_pattern.GetPtr());
    state_settings.depth.enabled        = m_settings.render_options.HasAnyBit(Option::DepthEnabled);
    state_settings.depth.write_enabled  = false;
    state_settings.depth.compare        = m_settings.render_options.HasAnyBit(Option::DepthReversed) ? Compare::GreaterEqual : Compare::Less;
    state_settings.rasterizer.is_front_counter_clockwise = true;

    m_render_state_ptr = Rhi::IRenderState::Create(m_context, state_settings);
    m_render_state_ptr->SetName("Sky-box render state");

    m_texture_sampler_ptr = Rhi::ISampler::Create(m_context, {
        Rhi::ISampler::Filter(Rhi::ISampler::Filter::MinMag::Linear),
        Rhi::ISampler::Address(Rhi::ISampler::Address::Mode::ClampToZero),
        Rhi::ISampler::LevelOfDetail(m_settings.lod_bias)
    });
    m_texture_sampler_ptr->SetName("Sky-box Texture Sampler");
}

Ptr<Rhi::IProgramBindings> SkyBox::CreateProgramBindings(const Ptr<Rhi::IBuffer>& uniforms_buffer_ptr, Data::Index frame_index) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(uniforms_buffer_ptr);
    META_CHECK_ARG_NOT_NULL(m_texture_sampler_ptr);
    META_CHECK_ARG_NOT_NULL(m_render_state_ptr);
    META_CHECK_ARG_NOT_NULL(m_render_state_ptr->GetSettings().program_ptr);
    return Rhi::IProgramBindings::Create(*m_render_state_ptr->GetSettings().program_ptr, {
        { { Rhi::ShaderType::Vertex, "g_skybox_uniforms" }, { { *uniforms_buffer_ptr         } } },
        { { Rhi::ShaderType::Pixel,  "g_skybox_texture"  }, { { m_mesh_buffers.GetTexture() } } },
        { { Rhi::ShaderType::Pixel,  "g_texture_sampler" }, { { *m_texture_sampler_ptr       } } },
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

void SkyBox::Draw(Rhi::IRenderCommandList& cmd_list, const MeshBufferBindings& buffer_bindings, Rhi::IViewState& view_state)
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
