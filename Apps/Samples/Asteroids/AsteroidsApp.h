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

FILE: AsteroidsApp.h
Sample demonstrating parallel rendering of the distinct asteroids massive

******************************************************************************/

#pragma once

#include "Asteroid.h"

#include <Methane/Kit.h>

namespace Methane::Samples
{

namespace gfx = Graphics;
namespace pal = Platform;

//#define ASTEROID_ARRAY_SIZE 100

struct AsteroidsFrame final : gfx::AppFrame
{
    struct MeshBufferBindings
    {
        gfx::Buffer::Ptr                    sp_uniforms_buffer;
        gfx::Program::ResourceBindings::Ptr sp_resource_bindings;
    };

    gfx::RenderCommandList::Ptr  sp_cmd_list;
    gfx::Buffer::Ptr             sp_scene_uniforms_buffer;
    MeshBufferBindings           skybox;
    MeshBufferBindings           asteroid;

    using gfx::AppFrame::AppFrame;
};

using GraphicsApp = gfx::App<AsteroidsFrame>;
class AsteroidsApp final : public GraphicsApp
{
public:
    AsteroidsApp();
    ~AsteroidsApp() override;

    // NativeApp
    void Init() override;
    bool Resize(const gfx::FrameSize& frame_size, bool is_minimized) override;
    void Update() override;
    void Render() override;

    // Context::Callback interface
    void OnContextReleased() override;

private:
    struct Vertex
    {
        gfx::Mesh::Position position;
        gfx::Mesh::Normal   normal;
        gfx::Mesh::TexCoord texcoord;

        using FieldsArray = std::array<gfx::Mesh::VertexField, 3>;
        static constexpr const FieldsArray layout = {
            gfx::Mesh::VertexField::Position,
            gfx::Mesh::VertexField::Normal,
            gfx::Mesh::VertexField::TexCoord,
        };
    };

    struct SHADER_STRUCT_ALIGN Constants
    {
        SHADER_FIELD_ALIGN gfx::Color     light_color;
        SHADER_FIELD_PACK  float          light_power;
        SHADER_FIELD_PACK  float          light_ambient_factor;
        SHADER_FIELD_PACK  float          light_specular_factor;
    };

    struct SHADER_STRUCT_ALIGN SceneUniforms
    {
        SHADER_FIELD_ALIGN gfx::Vector4f  eye_position;
        SHADER_FIELD_ALIGN gfx::Vector3f  light_position;
    };

    const float                 m_scene_scale;
    const Constants             m_scene_constants;
    gfx::ActionCamera           m_view_camera;
    gfx::ActionCamera           m_light_camera;

    SceneUniforms               m_scene_uniforms = { };
    gfx::SkyBox::Ptr            m_sp_sky_box;
    gfx::RenderState::Ptr       m_sp_state;
    gfx::Buffer::Ptr            m_sp_const_buffer;
    gfx::Sampler::Ptr           m_sp_texture_sampler;

#ifdef ASTEROID_ARRAY_SIZE
    AsteroidArray::Ptr          m_sp_asteroid_array;
#else
    Asteroid::Ptr               m_sp_asteroid;
#endif
};

} // namespace Methane::Samples
