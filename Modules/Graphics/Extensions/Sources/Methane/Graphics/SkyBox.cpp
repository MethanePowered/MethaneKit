/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

FILE: SkyBox.cpp
SkyBox rendering primitive

******************************************************************************/

#include <Methane/Graphics/SkyBox.h>
#include <Methane/Graphics/Mesh.h>
#include <Methane/Graphics/Buffer.h>
#include <Methane/Data/AppResourceProviders.h>

namespace Methane::Graphics
{

struct SkyBoxVertex
{
    Mesh::Position position;

    using FieldsArray = std::array<Mesh::VertexField, 1>;
    static constexpr const FieldsArray layout = {
        Mesh::VertexField::Position,
    };
};

SkyBox::SkyBox(Context& context, ImageLoader& image_loader, const Settings& settings)
    : m_settings(settings)
    , m_context(context)
    , m_mesh_buffers(context, SphereMesh<SkyBoxVertex>(Mesh::VertexLayoutFromArray(SkyBoxVertex::layout)), "Sky-Box")
{
    m_mesh_buffers.SetTexture(image_loader.LoadImagesToTextureCube(m_context, m_settings.face_resources, m_settings.mipmapped));

    const Context::Settings& context_settings = context.GetSettings();

    RenderState::Settings state_settings;
    state_settings.sp_program = Program::Create(context, {
        {
            Shader::CreateVertex(context, { Data::ShaderProvider::Get(), { "SkyBox", "SkyboxVS" }, { } }),
            Shader::CreatePixel( context, { Data::ShaderProvider::Get(), { "SkyBox", "SkyboxPS" }, { } }),
        },
        { { {
            { "in_position", "POSITION" },
        } } },
        { "g_skybox_texture", "g_texture_sampler" },
        { },
        { context_settings.color_format },
        context_settings.depth_stencil_format
    });
    state_settings.sp_program->SetName("Sky-box shading");
    state_settings.viewports     = { GetFrameViewport(context_settings.frame_size) };
    state_settings.scissor_rects = { GetFrameScissorRect(context_settings.frame_size) };
    state_settings.depth.enabled = false;
    state_settings.rasterizer.is_front_counter_clockwise = true;

    m_sp_state = RenderState::Create(context, state_settings);
    m_sp_state->SetName("Sky-box render state");

    m_sp_texture_sampler = Sampler::Create(context, {
        { Sampler::Filter::MinMag::Linear     },
        { Sampler::Address::Mode::ClampToZero },
        Sampler::LevelOfDetail(m_settings.lod_bias)
    });
    m_sp_texture_sampler->SetName("Sky-box Texture Sampler");
}

Program::ResourceBindings::Ptr SkyBox::CreateResourceBindings(const Buffer::Ptr& sp_uniforms_buffer)
{
    assert(!!m_sp_state);
    assert(!!m_sp_state->GetSettings().sp_program);
    return Program::ResourceBindings::Create(m_sp_state->GetSettings().sp_program, {
        { { Shader::Type::Vertex, "g_skybox_uniforms" }, { sp_uniforms_buffer             } },
        { { Shader::Type::Pixel,  "g_skybox_texture"  }, { m_mesh_buffers.GetTexturePtr() } },
        { { Shader::Type::Pixel,  "g_texture_sampler" }, { m_sp_texture_sampler           } },
    });
}

void SkyBox::Resize(const FrameSize& frame_size)
{
    assert(m_sp_state);
    m_sp_state->SetViewports({ GetFrameViewport(frame_size) });
    m_sp_state->SetScissorRects({ GetFrameScissorRect(frame_size) });
}

void SkyBox::Update()
{
    Matrix44f model_scale_matrix, model_translate_matrix, scene_view_matrix, scene_proj_matrix;
    m_settings.view_camera.GetViewProjMatrices(scene_view_matrix, scene_proj_matrix);
    cml::matrix_uniform_scale(model_scale_matrix, m_settings.scale);
    cml::matrix_translation(model_translate_matrix, m_settings.view_camera.GetOrientation().eye); // Sky-box is centered in the camera eye to simulate infinity distance

    m_mesh_buffers.SetFinalPassUniforms({ model_scale_matrix * model_translate_matrix * scene_view_matrix * scene_proj_matrix });
}

void SkyBox::Draw(RenderCommandList& cmd_list, Buffer& uniforms_buffer, Program::ResourceBindings& resource_bindings)
{
    assert(uniforms_buffer.GetDataSize() >= sizeof(MeshUniforms));
    uniforms_buffer.SetData({ { reinterpret_cast<Data::ConstRawPtr>(&m_mesh_buffers.GetFinalPassUniforms()), sizeof(MeshUniforms) } });

    cmd_list.Reset(*m_sp_state, "Sky-box rendering");
    m_mesh_buffers.Draw(cmd_list, resource_bindings);
}

} // namespace Methane::Graphics
