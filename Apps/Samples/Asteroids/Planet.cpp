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

FILE: Planet.cpp
Planet rendering primitive

******************************************************************************/

#include "Planet.h"

#include <Methane/Graphics/ImageLoader.h>
#include <Methane/Graphics/Mesh/SphereMesh.hpp>
#include <Methane/Graphics/Buffer.h>
#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Instrumentation.h>

namespace Methane::Samples
{

Planet::Planet(gfx::RenderContext& context, gfx::ImageLoader& image_loader, const Settings& settings)
    : Planet(context, image_loader, settings, gfx::SphereMesh<Vertex>(Vertex::layout, 1.f, 32, 32))
{
    META_FUNCTION_TASK();
}

Planet::Planet(gfx::RenderContext& context, gfx::ImageLoader& image_loader, const Settings& settings, gfx::BaseMesh<Vertex> mesh)
    : m_settings(settings)
    , m_context(context)
    , m_mesh_buffers(context, mesh, "Planet")
{
    META_FUNCTION_TASK();

    const gfx::RenderContext::Settings& context_settings = context.GetSettings();

    gfx::RenderState::Settings state_settings;
    state_settings.program_ptr = gfx::Program::Create(context,
        gfx::Program::Settings
        {
            gfx::Program::Shaders
            {
                gfx::Shader::CreateVertex(context, { Data::ShaderProvider::Get(), { "Planet", "PlanetVS" }, { } }),
                gfx::Shader::CreatePixel( context, { Data::ShaderProvider::Get(), { "Planet", "PlanetPS" }, { } }),
            },
            gfx::Program::InputBufferLayouts
            {
                gfx::Program::InputBufferLayout { mesh.GetVertexLayout().GetSemantics() }
            },
            gfx::Program::ArgumentDescriptions
            {
                { { gfx::Shader::Type::All,    "g_uniforms"  }, gfx::Program::Argument::Modifiers::None     },
                { { gfx::Shader::Type::Pixel,  "g_constants" }, gfx::Program::Argument::Modifiers::Constant },
                { { gfx::Shader::Type::Pixel,  "g_texture"   }, gfx::Program::Argument::Modifiers::Constant },
                { { gfx::Shader::Type::Pixel,  "g_sampler"   }, gfx::Program::Argument::Modifiers::Constant },
            },
            gfx::PixelFormats
            {
                context_settings.color_format
            },
            context_settings.depth_stencil_format
        }
    );
    state_settings.program_ptr->SetName("Planet Shaders");
    state_settings.depth.enabled = true;
    state_settings.depth.compare = m_settings.depth_reversed ? gfx::Compare::GreaterEqual : gfx::Compare::Less;
    m_render_state_ptr = gfx::RenderState::Create(context, state_settings);
    m_render_state_ptr->SetName("Planet Render State");
    
    m_mesh_buffers.SetTexture(image_loader.LoadImageToTexture2D(m_context, m_settings.texture_path, m_settings.image_options));

    m_texture_sampler_ptr = gfx::Sampler::Create(context, {
        { gfx::Sampler::Filter::MinMag::Linear     },
        { gfx::Sampler::Address::Mode::ClampToEdge },
        gfx::Sampler::LevelOfDetail(m_settings.lod_bias)
    });
    m_texture_sampler_ptr->SetName("Planet Texture Sampler");

    // Initialize default uniforms to be ready to render right away
    Update(0.0, 0.0);
}

Ptr<gfx::ProgramBindings> Planet::CreateProgramBindings(const Ptr<gfx::Buffer>& constants_buffer_ptr, const Ptr<gfx::Buffer>& uniforms_buffer_ptr)
{
    META_FUNCTION_TASK();

    assert(!!m_render_state_ptr);
    assert(!!m_render_state_ptr->GetSettings().program_ptr);
    return gfx::ProgramBindings::Create(m_render_state_ptr->GetSettings().program_ptr, {
        { { gfx::Shader::Type::All,   "g_uniforms"  }, { { uniforms_buffer_ptr            } } },
        { { gfx::Shader::Type::Pixel, "g_constants" }, { { constants_buffer_ptr           } } },
        { { gfx::Shader::Type::Pixel, "g_texture"   }, { { m_mesh_buffers.GetTexturePtr() } } },
        { { gfx::Shader::Type::Pixel, "g_sampler"   }, { { m_texture_sampler_ptr          } } },
    });
}

bool Planet::Update(double elapsed_seconds, double)
{
    META_FUNCTION_TASK();

    gfx::Matrix44f model_scale_matrix, model_translate_matrix, model_rotation_matrix;
    cml::matrix_uniform_scale(model_scale_matrix, m_settings.scale);
    cml::matrix_translation(model_translate_matrix, m_settings.position);
    cml::matrix_rotation_world_y(model_rotation_matrix, -m_settings.spin_velocity_rps * elapsed_seconds);

    Uniforms uniforms{};
    uniforms.eye_position   = gfx::Vector4f(m_settings.view_camera.GetOrientation().eye, 1.f);
    uniforms.light_position = m_settings.light_camera.GetOrientation().eye;
    uniforms.model_matrix   = model_scale_matrix * model_rotation_matrix * model_translate_matrix;
    uniforms.mvp_matrix     = uniforms.model_matrix * m_settings.view_camera.GetViewProjMatrix();

    m_mesh_buffers.SetFinalPassUniforms(std::move(uniforms));
    return true;
}

void Planet::Draw(gfx::RenderCommandList& cmd_list, gfx::MeshBufferBindings& buffer_bindings, gfx::ViewState& view_state)
{
    META_FUNCTION_TASK();
    META_DEBUG_GROUP_CREATE_VAR(s_debug_group, "Planet rendering");

    assert(!!buffer_bindings.uniforms_buffer_ptr);
    assert(buffer_bindings.uniforms_buffer_ptr->GetDataSize() >= sizeof(Uniforms));
    buffer_bindings.uniforms_buffer_ptr->SetData(m_mesh_buffers.GetFinalPassUniformsSubresources());

    cmd_list.Reset(m_render_state_ptr, s_debug_group.get());
    cmd_list.SetViewState(view_state);
    
    assert(!buffer_bindings.program_bindings_per_instance.empty());
    assert(!!buffer_bindings.program_bindings_per_instance[0]);
    m_mesh_buffers.Draw(cmd_list, *buffer_bindings.program_bindings_per_instance[0]);
}

} // namespace Methane::Graphics
