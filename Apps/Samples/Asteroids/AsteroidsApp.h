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
    Ptr<gfx::RenderPass>                sp_initial_screen_pass;
    Ptr<gfx::RenderPass>                sp_final_screen_pass;
    Ptr<gfx::ParallelRenderCommandList> sp_parallel_cmd_list;
    Ptr<gfx::RenderCommandList>         sp_serial_cmd_list;
    Ptr<gfx::RenderCommandList>         sp_final_cmd_list;
    Ptr<gfx::Buffer>                    sp_scene_uniforms_buffer;
    gfx::MeshBufferBindings             skybox;
    gfx::MeshBufferBindings             planet;
    gfx::MeshBufferBindings             asteroids;

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
    bool Update() override;
    bool Render() override;

    // Context::Callback overrides
    void OnContextReleased() override;

    uint32_t GetAsteroidsComplexity() const { return m_asteroids_complexity; }
    void     SetAsteroidsComplexity(uint32_t asteroids_complexity);
    
    bool     IsParallelRenderingEnabled() const { return m_is_parallel_rendering_enabled; }
    void     SetParallelRenderingEnabled(bool is_parallel_rendering_enabled);

    AsteroidsArray& GetAsteroidsArray() const;
    std::string     GetParametersString() const;

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

    gfx::ActionCamera                 m_view_camera;
    gfx::ActionCamera                 m_light_camera;
    const float                       m_scene_scale;
    const Constants                   m_scene_constants;
    AsteroidsArray::Settings          m_asteroids_array_settings;
    uint32_t                          m_asteroids_complexity          = 0u;
    bool                              m_is_parallel_rendering_enabled = true;
    SceneUniforms                     m_scene_uniforms = { };
    
    Ptr<gfx::Buffer>                  m_sp_const_buffer;
    Ptr<gfx::SkyBox>                  m_sp_sky_box;
    Ptr<Planet>                       m_sp_planet;
    Ptr<AsteroidsArray>               m_sp_asteroids_array;
    Ptr<AsteroidsArray::ContentState> m_sp_asteroids_array_state;
};

} // namespace Methane::Samples
