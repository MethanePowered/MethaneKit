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
    , m_mesh_buffers(context, BoxMesh<SkyBoxVertex>(Mesh::VertexLayoutFromArray(SkyBoxVertex::layout)), "Sky-Box")
{
    m_mesh_buffers.SetTexture(image_loader.LoadImagesToTextureCube(m_context, m_settings.face_resources));

    const Context::Settings& context_settings = context.GetSettings();

    RenderState::Settings state_settings;
    state_settings.sp_program = Program::Create(context, {
        {
            Shader::CreateVertex(context, { Data::ShaderProvider::Get(), { "SkyBox", "SkyboxVS", "vs_5_1" }, { } }),
            Shader::CreatePixel( context, { Data::ShaderProvider::Get(), { "SkyBox", "SkyboxPS", "ps_5_1" }, { } }),
        },
        { { {
            { "in_position", "POSITION" },
        } } },
        { "g_skybox_texture", "g_texture_sampler" },
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
}

void SkyBox::Draw(RenderCommandList& cmd_list, Program::ResourceBindings& resource_bindings)
{
    cmd_list.Reset(*m_sp_state, "Sky-box rendering");
    m_mesh_buffers.Draw(cmd_list, resource_bindings, 1);
}

} // namespace Methane::Graphics
