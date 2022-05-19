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

FILE: AsteroidsApp.h
Sample demonstrating parallel rendering of the distinct asteroids massive

******************************************************************************/

#pragma once

#include "Planet.h"
#include "AsteroidsArray.h"

#include <Methane/Kit.h>
#include <Methane/UserInterface/App.hpp>

namespace hlslpp // NOSONAR
{
#pragma pack(push, 16)
#include "Shaders/SceneConstants.h"   // NOSONAR
#include "Shaders/AsteroidUniforms.h" // NOSONAR
#pragma pack(pop)
}

namespace Methane::Samples
{

namespace gfx = Graphics;
namespace pal = Platform;

struct AsteroidsFrame final : gfx::AppFrame
{
    Ptr<gfx::RenderPass>                asteroids_pass_ptr;
    Ptr<gfx::ParallelRenderCommandList> parallel_cmd_list_ptr;
    Ptr<gfx::RenderCommandList>         serial_cmd_list_ptr;
    Ptr<gfx::RenderCommandList>         final_cmd_list_ptr;
    Ptr<gfx::CommandListSet>            execute_cmd_list_set_ptr;
    Ptr<gfx::Buffer>                    scene_uniforms_buffer_ptr;
    gfx::MeshBufferBindings             sky_box;
    gfx::MeshBufferBindings             planet;
    gfx::InstancedMeshBufferBindings    asteroids;

    using gfx::AppFrame::AppFrame;

    // AppFrame overrides
    void ReleaseScreenPassAttachmentTextures() override;
};

using UserInterfaceApp = UserInterface::App<AsteroidsFrame>;
class AsteroidsApp final : public UserInterfaceApp // NOSONAR
{
public:
    AsteroidsApp();
    ~AsteroidsApp() override;

    // NativeApp
    void Init() override;
    bool Resize(const gfx::FrameSize& frame_size, bool is_minimized) override;
    bool Update() override;
    bool Render() override;

    // UserInterface::App overrides
    std::string GetParametersString() override;

    uint32_t GetAsteroidsComplexity() const { return m_asteroids_complexity; }
    void     SetAsteroidsComplexity(uint32_t asteroids_complexity);

    bool     IsParallelRenderingEnabled() const { return m_is_parallel_rendering_enabled; }
    void     SetParallelRenderingEnabled(bool is_parallel_rendering_enabled);

    AsteroidsArray& GetAsteroidsArray() const;

protected:
    // IContextCallback overrides
    void OnContextReleased(gfx::Context& context) override;

private:
    bool Animate(double elapsed_seconds, double delta_seconds) const;
    Ptr<gfx::CommandListSet> CreateExecuteCommandListSet(const AsteroidsFrame& frame) const;

    gfx::ActionCamera                 m_view_camera;
    gfx::ActionCamera                 m_light_camera;
    const float                       m_scene_scale = 15.F;
    const hlslpp::SceneConstants      m_scene_constants{
        { 1.F, 1.F, 1.F, 1.F },       // - light_color
        3.0F,                         // - light_power
        0.05F,                        // - light_ambient_factor
        30.F                          // - light_specular_factor
    };
    AsteroidsArray::Settings          m_asteroids_array_settings;
    uint32_t                          m_asteroids_complexity          = 0U;
    bool                              m_is_parallel_rendering_enabled = true;
    hlslpp::SceneUniforms             m_scene_uniforms{ };
    gfx::Resource::SubResources       m_scene_uniforms_subresources{
        { reinterpret_cast<Data::ConstRawPtr>(&m_scene_uniforms), sizeof(hlslpp::SceneUniforms) } // NOSONAR
    };

    Ptr<gfx::RenderPattern>           m_asteroids_render_pattern_ptr;
    Ptr<gfx::Buffer>                  m_const_buffer_ptr;
    Ptr<gfx::SkyBox>                  m_sky_box_ptr;
    Ptr<Planet>                       m_planet_ptr;
    Ptr<AsteroidsArray>               m_asteroids_array_ptr;
    Ptr<AsteroidsArray::ContentState> m_asteroids_array_state_ptr;
};

} // namespace Methane::Samples
