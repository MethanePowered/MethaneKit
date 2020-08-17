/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/SkyBox.cpp
SkyBox rendering primitive

******************************************************************************/

#include <Methane/Graphics/SkyBox.h>
#include <Methane/Graphics/Mesh/SphereMesh.hpp>
#include <Methane/Graphics/Buffer.h>
#include <Methane/Graphics/Camera.h>
#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

SkyBox::SkyBox(RenderContext& context, ImageLoader& image_loader, const Settings& settings)
    : SkyBox(context, image_loader, settings, SphereMesh<Vertex>(Vertex::layout))
{
    META_FUNCTION_TASK();
}

SkyBox::SkyBox(RenderContext& context, ImageLoader& image_loader, const Settings& settings, BaseMesh<Vertex> mesh)
    : m_settings(settings)
    , m_context(context)
    , m_mesh_buffers(context, mesh, "Sky-Box")
{
    META_FUNCTION_TASK();

    m_mesh_buffers.SetTexture(image_loader.LoadImagesToTextureCube(m_context, m_settings.face_resources, m_settings.image_options));

    const RenderContext::Settings& context_settings = context.GetSettings();

    RenderState::Settings state_settings;
    state_settings.sp_program = Program::Create(context,
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
                { { Shader::Type::Vertex, "g_skybox_uniforms" }, Program::Argument::Modifiers::None },
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
    state_settings.sp_program->SetName("Sky-box shading");
    state_settings.depth.enabled        = m_settings.render_options & Options::DepthEnabled;
    state_settings.depth.write_enabled  = false;
    state_settings.depth.compare        = m_settings.render_options & Options::DepthReversed ? Compare::GreaterEqual : Compare::Less;
    state_settings.rasterizer.is_front_counter_clockwise = true;

    m_sp_render_state = RenderState::Create(context, state_settings);
    m_sp_render_state->SetName("Sky-box render state");

    m_sp_texture_sampler = Sampler::Create(context, {
        { Sampler::Filter::MinMag::Linear     },
        { Sampler::Address::Mode::ClampToZero },
        Sampler::LevelOfDetail(m_settings.lod_bias)
    });
    m_sp_texture_sampler->SetName("Sky-box Texture Sampler");
}

Ptr<ProgramBindings> SkyBox::CreateProgramBindings(const Ptr<Buffer>& sp_uniforms_buffer)
{
    META_FUNCTION_TASK();

    assert(!!m_sp_render_state);
    assert(!!m_sp_render_state->GetSettings().sp_program);
    return ProgramBindings::Create(m_sp_render_state->GetSettings().sp_program, {
        { { Shader::Type::Vertex, "g_skybox_uniforms" }, { { sp_uniforms_buffer             } } },
        { { Shader::Type::Pixel,  "g_skybox_texture"  }, { { m_mesh_buffers.GetTexturePtr() } } },
        { { Shader::Type::Pixel,  "g_texture_sampler" }, { { m_sp_texture_sampler           } } },
    });
}

void SkyBox::Update()
{
    META_FUNCTION_TASK();

    Matrix44f model_scale_matrix, model_translate_matrix;
    cml::matrix_uniform_scale(model_scale_matrix, m_settings.scale);
    cml::matrix_translation(model_translate_matrix, m_settings.view_camera.GetOrientation().eye); // Sky-box is centered in the camera eye to simulate infinity distance

    m_mesh_buffers.SetFinalPassUniforms({ model_scale_matrix * model_translate_matrix * m_settings.view_camera.GetViewProjMatrix() });
}

void SkyBox::Draw(RenderCommandList& cmd_list, MeshBufferBindings& buffer_bindings, ViewState& view_state)
{
    META_FUNCTION_TASK();
    META_DEBUG_GROUP_CREATE_VAR(s_debug_group, "Sky-box rendering");
    
    assert(!!buffer_bindings.sp_uniforms_buffer);
    assert(buffer_bindings.sp_uniforms_buffer->GetDataSize() >= sizeof(Uniforms));
    buffer_bindings.sp_uniforms_buffer->SetData(m_mesh_buffers.GetFinalPassUniformsSubresources());

    cmd_list.Reset(m_sp_render_state, s_debug_group.get());
    cmd_list.SetViewState(view_state);
    
    assert(!buffer_bindings.program_bindings_per_instance.empty());
    assert(!!buffer_bindings.program_bindings_per_instance[0]);
    m_mesh_buffers.Draw(cmd_list, *buffer_bindings.program_bindings_per_instance[0]);
}

} // namespace Methane::Graphics
