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

#include "Planet.h"
#include "AsteroidsArray.h"

#include <Methane/Graphics/Kit.h>

namespace Methane::Samples
{

namespace gfx = Graphics;
namespace pal = Platform;

struct AsteroidsFrame final : gfx::AppFrame
{
    struct MeshBufferBindings
    {
        using ResourceBindingsArray = std::vector<gfx::Program::ResourceBindings::Ptr>;
        
        gfx::Buffer::Ptr      sp_uniforms_buffer;
        ResourceBindingsArray resource_bindings_array;
    };

    gfx::RenderCommandList::Ptr  sp_cmd_list;
    gfx::Buffer::Ptr             sp_scene_uniforms_buffer;
    MeshBufferBindings           skybox;
    MeshBufferBindings           planet;
    MeshBufferBindings           asteroids;

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
    struct SHADER_STRUCT_ALIGN Constants
    {
        SHADER_FIELD_ALIGN gfx::Color4f   light_color;
        SHADER_FIELD_PACK  float          light_power;
        SHADER_FIELD_PACK  float          light_ambient_factor;
        SHADER_FIELD_PACK  float          light_specular_factor;
    };

    struct SHADER_STRUCT_ALIGN SceneUniforms
    {
        SHADER_FIELD_ALIGN gfx::Vector4f  eye_position;
        SHADER_FIELD_ALIGN gfx::Vector3f  light_position;
    };

    gfx::ActionCamera               m_view_camera;
    gfx::ActionCamera               m_light_camera;
    const float                     m_scene_scale;
    const Constants                 m_scene_constants;
    const AsteroidsArray::Settings  m_asteroids_array_settings;

    SceneUniforms                   m_scene_uniforms = { };
    gfx::RenderState::Ptr           m_sp_state;
    gfx::Buffer::Ptr                m_sp_const_buffer;
    gfx::Sampler::Ptr               m_sp_texture_sampler;
    gfx::SkyBox::Ptr                m_sp_sky_box;
    Planet::Ptr                     m_sp_planet;
    AsteroidsArray::Ptr             m_sp_asteroids_array;
    AsteroidsArray::State::Ptr      m_sp_asteroids_array_state;
};

} // namespace Methane::Samples
