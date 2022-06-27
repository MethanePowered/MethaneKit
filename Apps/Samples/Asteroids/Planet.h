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

FILE: Planet.h
Planet rendering primitive

******************************************************************************/

#pragma once

#include <Methane/Graphics/MeshBuffers.hpp>
#include <Methane/Graphics/Camera.h>
#include <Methane/Graphics/RenderState.h>
#include <Methane/Graphics/Buffer.h>
#include <Methane/Graphics/Program.h>
#include <Methane/Graphics/Sampler.h>
#include <Methane/Graphics/Types.h>
#include <Methane/Graphics/Mesh.h>
#include <Methane/Graphics/ImageLoader.h>

namespace hlslpp // NOSONAR
{
#pragma pack(push, 16)
#include "Shaders/PlanetUniforms.h" // NOSONAR
#pragma pack(pop)
}

#include <memory>

namespace Methane::Samples
{

namespace gfx = Graphics;

struct RenderPattern;

class Planet
{
public:
    struct Settings
    {
        const gfx::Camera&        view_camera;
        const gfx::Camera&        light_camera;
        std::string               texture_path;
        hlslpp::float3            position;
        float                     scale;
        float                     spin_velocity_rps   = 0.3F; // (rps = radians per second)
        bool                      depth_reversed      = false;
        gfx::ImageLoader::Options image_options       = gfx::ImageLoader::Options::None;
        float                     lod_bias            = 0.F;
    };

    Planet(gfx::CommandQueue& render_cmd_queue, gfx::RenderPattern& render_pattern, const gfx::ImageLoader& image_loader, const Settings& settings);

    Ptr<gfx::ProgramBindings> CreateProgramBindings(const Ptr<gfx::Buffer>& constants_buffer_ptr, const Ptr<gfx::Buffer>& uniforms_buffer_ptr, Data::Index frame_index) const;
    bool Update(double elapsed_seconds, double delta_seconds);
    void Draw(gfx::RenderCommandList& cmd_list, const gfx::MeshBufferBindings& buffer_bindings, gfx::ViewState& view_state);

private:
    using TexturedMeshBuffers = gfx::TexturedMeshBuffers<hlslpp::PlanetUniforms>;

    struct Vertex
    {
        gfx::Mesh::Position position;
        gfx::Mesh::Normal   normal;
        gfx::Mesh::TexCoord texcoord;

        inline static const gfx::Mesh::VertexLayout layout{
            gfx::Mesh::VertexField::Position,
            gfx::Mesh::VertexField::Normal,
            gfx::Mesh::VertexField::TexCoord,
        };
    };

    Planet(gfx::CommandQueue& render_cmd_queue, gfx::RenderPattern& render_pattern, const gfx::ImageLoader& image_loader, const Settings& settings, const gfx::BaseMesh<Vertex>& mesh);

    Settings                     m_settings;
    gfx::RenderContext&          m_context;
    const Ptr<gfx::CommandQueue> m_render_cmd_queue_ptr;
    TexturedMeshBuffers          m_mesh_buffers;
    Ptr<gfx::Sampler>            m_texture_sampler_ptr;
    Ptr<gfx::RenderState>        m_render_state_ptr;
};

} // namespace Methane::Graphics
