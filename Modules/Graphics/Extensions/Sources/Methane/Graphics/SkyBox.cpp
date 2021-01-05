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
#include <Methane/Graphics/Mesh/SphereMesh.hpp>
#include <Methane/Graphics/Buffer.h>
#include <Methane/Graphics/Camera.h>
#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Instrumentation.h>

#include <magic_enum.hpp>

namespace Methane::Graphics
{

SkyBox::SkyBox(RenderContext& context, const ImageLoader& image_loader, const Settings& settings)
    : SkyBox(context, image_loader, settings, SphereMesh<Vertex>(Vertex::layout))
{
    META_FUNCTION_TASK();
}

SkyBox::SkyBox(RenderContext& context, const ImageLoader& image_loader, const Settings& settings, const BaseMesh<Vertex>& mesh)
    : m_settings(settings)
    , m_context(context)
    , m_mesh_buffers(context, mesh, "Sky-Box")
{
    META_FUNCTION_TASK();

    m_mesh_buffers.SetTexture(image_loader.LoadImagesToTextureCube(m_context, m_settings.face_resources, m_settings.image_options));

    const RenderContext::Settings& context_settings = context.GetSettings();

    RenderState::Settings state_settings;
    state_settings.program_ptr = Program::Create(context,
        Program::Settings
        {
            Program::Shaders
            {
                Shader::CreateVertex(context, { Data::ShaderProvider::Get(), { "SkyBox", "SkyboxVS" }, { } }),
                Shader::CreatePixel( context, { Data::ShaderProvider::Get(), { "SkyBox", "SkyboxPS" }, { } }),
            },
            Program::InputBufferLayouts
            {
                Program::InputBufferLayout
                {
                    Program::InputBufferLayout::ArgumentSemantics { mesh.GetVertexLayout().GetSemantics() }
                }
            },
            Program::ArgumentDescriptions
            {
                { { Shader::Type::Vertex, "g_skybox_uniforms" }, Program::Argument::Modifiers::None     },
                { { Shader::Type::Pixel,  "g_skybox_texture"  }, Program::Argument::Modifiers::Constant },
                { { Shader::Type::Pixel,  "g_texture_sampler" }, Program::Argument::Modifiers::Constant },
            },
            PixelFormats
            {
                context_settings.color_format
            },
            context_settings.depth_stencil_format
        }
    );

    using namespace magic_enum::bitwise_operators;
    state_settings.program_ptr->SetName("Sky-box shading");
    state_settings.depth.enabled        = magic_enum::flags::enum_contains(m_settings.render_options & Options::DepthEnabled);
    state_settings.depth.write_enabled  = false;
    state_settings.depth.compare        = magic_enum::flags::enum_contains(m_settings.render_options & Options::DepthReversed) ? Compare::GreaterEqual : Compare::Less;
    state_settings.rasterizer.is_front_counter_clockwise = true;

    m_render_state_ptr = RenderState::Create(context, state_settings);
    m_render_state_ptr->SetName("Sky-box render state");

    m_texture_sampler_ptr = Sampler::Create(context, {
        Sampler::Filter(Sampler::Filter::MinMag::Linear),
        Sampler::Address(Sampler::Address::Mode::ClampToZero),
        Sampler::LevelOfDetail(m_settings.lod_bias)
    });
    m_texture_sampler_ptr->SetName("Sky-box Texture Sampler");
}

Ptr<ProgramBindings> SkyBox::CreateProgramBindings(const Ptr<Buffer>& uniforms_buffer_ptr) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_render_state_ptr);
    META_CHECK_ARG_NOT_NULL(m_render_state_ptr->GetSettings().program_ptr);
    return ProgramBindings::Create(m_render_state_ptr->GetSettings().program_ptr, {
        { { Shader::Type::Vertex, "g_skybox_uniforms" }, { { uniforms_buffer_ptr            } } },
        { { Shader::Type::Pixel,  "g_skybox_texture"  }, { { m_mesh_buffers.GetTexturePtr() } } },
        { { Shader::Type::Pixel,  "g_texture_sampler" }, { { m_texture_sampler_ptr          } } },
    });
}

void SkyBox::Update()
{
    META_FUNCTION_TASK();
    m_mesh_buffers.SetFinalPassUniforms({
        hlslpp::transpose(hlslpp::mul(
            hlslpp::mul(
                hlslpp::float4x4_scale(m_settings.scale),
                hlslpp::float4x4_translate(m_settings.view_camera.GetOrientation().eye)),
            m_settings.view_camera.GetViewProjMatrix()
        ))
    });
}

void SkyBox::Draw(RenderCommandList& cmd_list, MeshBufferBindings& buffer_bindings, ViewState& view_state)
{
    META_FUNCTION_TASK();
    META_DEBUG_GROUP_CREATE_VAR(s_debug_group, "Sky-box rendering");
    
    META_CHECK_ARG_NOT_NULL(buffer_bindings.uniforms_buffer_ptr);
    META_CHECK_ARG_GREATER_OR_EQUAL(buffer_bindings.uniforms_buffer_ptr->GetDataSize(), sizeof(Uniforms));
    buffer_bindings.uniforms_buffer_ptr->SetData(m_mesh_buffers.GetFinalPassUniformsSubresources());

    cmd_list.ResetWithStateOnce(*m_render_state_ptr, s_debug_group.get());
    cmd_list.SetViewState(view_state);
    
    META_CHECK_ARG_NOT_EMPTY(buffer_bindings.program_bindings_per_instance);
    META_CHECK_ARG_NOT_NULL(buffer_bindings.program_bindings_per_instance[0]);
    m_mesh_buffers.Draw(cmd_list, *buffer_bindings.program_bindings_per_instance[0]);
}

} // namespace Methane::Graphics
