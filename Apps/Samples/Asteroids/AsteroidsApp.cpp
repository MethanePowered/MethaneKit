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

FILE: AsteroidsApp.cpp
Sample demonstrating parallel rendering of the distinct asteroids massive

******************************************************************************/

#include "AsteroidsApp.h"
#include "AsteroidsAppController.h"

#include <Methane/Samples/AppSettings.hpp>
#include <Methane/Graphics/AppCameraController.h>
#include <Methane/Data/TimeAnimation.h>
#include <Methane/Instrumentation.h>

#include <cassert>
#include <memory>
#include <thread>
#include <array>
#include <map>

namespace Methane::Samples
{

struct MutableParameters
{
    uint32_t instances_count;
    uint32_t unique_mesh_count;
    uint32_t textures_count;
    float    scale_ratio;
};

constexpr uint32_t g_max_complexity = 9;
static const std::array<MutableParameters, g_max_complexity+1> g_mutable_parameters{ {
    { 1000U,  35U,   10U, 0.6F  }, // 0
    { 2000U,  50U,   10U, 0.5F  }, // 1
    { 3000U,  75U,   20U, 0.45F }, // 2
    { 4000U,  100U,  20U, 0.4F  }, // 3
    { 5000U,  200U,  30U, 0.33F }, // 4
    { 10000U, 300U,  30U, 0.3F  }, // 5
    { 15000U, 400U,  40U, 0.27F }, // 6
    { 20000U, 500U,  40U, 0.23F }, // 7
    { 35000U, 750U,  50U, 0.2F  }, // 8
    { 50000U, 1000U, 50U, 0.17F }, // 9
} };

inline uint32_t GetDefaultComplexity()
{
#ifdef _DEBUG
    return 1U;
#else
    return std::thread::hardware_concurrency() / 2;
#endif
}

inline const MutableParameters& GetMutableParameters(uint32_t complexity)
{
    return g_mutable_parameters[std::min(complexity, g_max_complexity)];
}

inline const MutableParameters& GetMutableParameters()
{
    return GetMutableParameters(GetDefaultComplexity());
}

static const std::map<pal::Keyboard::State, AsteroidsAppAction> g_asteroids_action_by_keyboard_state{
    { { pal::Keyboard::Key::RightBracket }, AsteroidsAppAction::IncreaseComplexity          },
    { { pal::Keyboard::Key::LeftBracket  }, AsteroidsAppAction::DecreaseComplexity          },
    { { pal::Keyboard::Key::P            }, AsteroidsAppAction::SwitchParallelRendering     },
    { { pal::Keyboard::Key::L            }, AsteroidsAppAction::SwitchMeshLodsColoring      },
    { { pal::Keyboard::Key::Apostrophe   }, AsteroidsAppAction::IncreaseMeshLodComplexity   },
    { { pal::Keyboard::Key::Semicolon    }, AsteroidsAppAction::DecreaseMeshLodComplexity   },
};

void AsteroidsFrame::ReleaseScreenPassAttachmentTextures()
{
    META_FUNCTION_TASK();
    initial_screen_pass_ptr->ReleaseAttachmentTextures();
    final_screen_pass_ptr->ReleaseAttachmentTextures();
    AppFrame::ReleaseScreenPassAttachmentTextures();
}

AsteroidsApp::AsteroidsApp()
    : UserInterfaceApp(
        Samples::GetGraphicsAppSettings("Methane Asteroids", true /* animations */, true /* depth */, 0.F /* depth clear */, { /* color clearing disabled */ }),
        { HeadsUpDisplayMode::UserInterface, true },
        "Methane Asteroids sample is demonstrating parallel rendering\nof massive asteroids field dynamic simulation.")
    , m_view_camera(GetAnimations(), gfx::ActionCamera::Pivot::Aim)
    , m_light_camera(m_view_camera, GetAnimations(), gfx::ActionCamera::Pivot::Aim)
    , m_scene_scale(15.F)
    , m_scene_constants(                                // Shader constants:
        {                                               // ================
            gfx::Color4f(1.F, 1.F, 1.F, 1.F),           // - light_color
            3.0F,                                       // - light_power
            0.05F,                                      // - light_ambient_factor
            30.F                                        // - light_specular_factor
        })
    , m_asteroids_array_settings(                       // Asteroids array settings:
        {                                               // ================
            m_view_camera,                              // - view_camera
            m_scene_scale,                              // - scale
            GetMutableParameters().instances_count,     // - instance_count
            GetMutableParameters().unique_mesh_count,   // - unique_mesh_count
            4U,                                         // - subdivisions_count
            GetMutableParameters().textures_count,      // - textures_count
            { 256U, 256U },                             // - texture_dimensions
            1123U,                                      // - random_seed
            13.F,                                       // - orbit_radius_ratio
            4.F,                                        // - disc_radius_ratio
            0.06F,                                      // - mesh_lod_min_screen_size
            GetMutableParameters().scale_ratio / 10.F,  // - min_asteroid_scale_ratio
            GetMutableParameters().scale_ratio,         // - max_asteroid_scale_ratio
            true,                                       // - textures_array_enabled
            true                                        // - depth_reversed
        })
    , m_asteroids_complexity(static_cast<uint32_t>(GetDefaultComplexity()))
    , m_is_parallel_rendering_enabled(true)
{
    META_FUNCTION_TASK();

    // NOTE: Near and Far values are swapped in camera parameters (1st value is near = max depth, 2nd value is far = min depth)
    // for Reversed-Z buffer values range [ near: 1, far 0], instead of [ near 0, far 1]
    // which is used for "from near to far" drawing order for reducing pixels overdraw
    m_view_camera.ResetOrientation({ { -110.F, 75.F, 210.F }, { 0.F, -60.F, 25.F }, { 0.F, 1.F, 0.F } });
    m_view_camera.SetParameters({ 600.F /* near = max depth */, 0.01F /*far = min depth*/, 90.F /* FOV */ });
    m_view_camera.SetZoomDistanceRange({ 60.F , 400.F });

    m_light_camera.ResetOrientation({ { -100.F, 120.F, 0.F }, { 0.F, 0.F, 0.F }, { 0.F, 1.F, 0.F } });
    m_light_camera.SetProjection(gfx::Camera::Projection::Orthogonal);
    m_light_camera.SetParameters({ -300.F, 300.F, 90.F });
    m_light_camera.Resize({ 120.F, 120.F });

    AddInputControllers({
        std::make_shared<AsteroidsAppController>(*this, g_asteroids_action_by_keyboard_state),
        std::make_shared<gfx::AppCameraController>(m_view_camera,  "VIEW CAMERA"),
        std::make_shared<gfx::AppCameraController>(m_light_camera, "LIGHT SOURCE",
            gfx::AppCameraController::ActionByMouseButton   { { pal::Mouse::Button::Right, gfx::ActionCamera::MouseAction::Rotate   } },
            gfx::AppCameraController::ActionByKeyboardState { { { pal::Keyboard::Key::LeftControl, pal::Keyboard::Key::L }, gfx::ActionCamera::KeyboardAction::Reset } },
            gfx::AppCameraController::ActionByKeyboardKey   { }),
    });

    const std::string options_group = "Asteroids Options";
    add_option_group(options_group);
    add_option("-c,--complexity",
               [this](CLI::results_t res) {
                       uint32_t complexity = 0;
                       if (CLI::detail::lexical_cast(res[0], complexity))
                       {
                           SetAsteroidsComplexity(complexity);
                           return true;
                       }
                       return false;
                   }, "simulation complexity", true)
        ->default_val(m_asteroids_complexity)
        ->expected(0, static_cast<int>(g_max_complexity))
        ->group(options_group);
    add_option("-s,--subdiv-count", m_asteroids_array_settings.subdivisions_count, "mesh subdivisions count", true)->group(options_group);
    add_option("-t,--texture-array", m_asteroids_array_settings.textures_array_enabled, "texture array enabled", true)->group(options_group);
    add_option("-r,--parallel-render", m_is_parallel_rendering_enabled, "parallel rendering enabled", true)->group(options_group);

    // Setup animations
    GetAnimations().push_back(std::make_shared<Data::TimeAnimation>(std::bind(&AsteroidsApp::Animate, this, std::placeholders::_1, std::placeholders::_2)));

    // Enable dry updates on pause to keep asteroids in sync with projection matrix dependent on window size which may change
    GetAnimations().SetDryUpdateOnPauseEnabled(true);

    ShowParameters();
}

AsteroidsApp::~AsteroidsApp()
{
    META_FUNCTION_TASK();
    // Wait for GPU rendering is completed to release resources
    GetRenderContext().WaitForGpu(gfx::RenderContext::WaitFor::RenderComplete);
}

void AsteroidsApp::Init()
{
    META_FUNCTION_TASK();
    META_SCOPE_TIMER("AsteroidsApp::Init");

    UserInterfaceApp::Init();

    gfx::RenderContext& context = GetRenderContext();
    const gfx::RenderContext::Settings& context_settings = context.GetSettings();
    const Data::FloatSize float_rect_size(static_cast<float>(context_settings.frame_size.width),
                                          static_cast<float>(context_settings.frame_size.height));
    m_view_camera.Resize(float_rect_size);

    // Create sky-box
    m_sky_box_ptr = std::make_shared<gfx::SkyBox>(context, GetImageLoader(), gfx::SkyBox::Settings{
        m_view_camera,
        {
            "Textures/SkyBox/Galaxy/PositiveX.jpg",
            "Textures/SkyBox/Galaxy/NegativeX.jpg",
            "Textures/SkyBox/Galaxy/PositiveY.jpg",
            "Textures/SkyBox/Galaxy/NegativeY.jpg",
            "Textures/SkyBox/Galaxy/PositiveZ.jpg",
            "Textures/SkyBox/Galaxy/NegativeZ.jpg"
        },
        m_scene_scale * 100.F,
        gfx::ImageLoader::Options::Mipmapped,
        gfx::SkyBox::Options::DepthEnabled | gfx::SkyBox::Options::DepthReversed
    });

    // Create planet
    m_planet_ptr = std::make_shared<Planet>(context, GetImageLoader(), Planet::Settings{
        m_view_camera,
        m_light_camera,
        "Textures/Planet/Mars.jpg",             // texture_path
        gfx::Vector3f(0.F, 0.F, 0.F),           // position
        m_scene_scale * 3.F,                    // scale
        0.1F,                                   // spin_velocity_rps
        true,                                   // depth_reversed
        gfx::ImageLoader::Options::Mipmapped |  // image_options
        gfx::ImageLoader::Options::SrgbColorSpace,
        -1.F,                                   // lod_bias
    });

    // Create asteroids array
    m_asteroids_array_ptr = m_asteroids_array_state_ptr
                         ? std::make_unique<AsteroidsArray>(context, m_asteroids_array_settings, *m_asteroids_array_state_ptr)
                         : std::make_unique<AsteroidsArray>(context, m_asteroids_array_settings);

    const Data::Size constants_data_size         = gfx::Buffer::GetAlignedBufferSize(static_cast<Data::Size>(sizeof(Constants)));
    const Data::Size scene_uniforms_data_size    = gfx::Buffer::GetAlignedBufferSize(static_cast<Data::Size>(sizeof(SceneUniforms)));
    const Data::Size asteroid_uniforms_data_size = m_asteroids_array_ptr->GetUniformsBufferSize();

    // Create constants buffer for frame rendering
    m_const_buffer_ptr = gfx::Buffer::CreateConstantBuffer(context, constants_data_size);
    m_const_buffer_ptr->SetName("Constants Buffer");
    m_const_buffer_ptr->SetData({ { reinterpret_cast<Data::ConstRawPtr>(&m_scene_constants), sizeof(m_scene_constants) } });

    // ========= Per-Frame Data =========
    for(AsteroidsFrame& frame : GetFrames())
    {
        // Create initial screen pass for asteroids rendering
        gfx::RenderPass::Settings initial_screen_pass_settings = frame.screen_pass_ptr->GetSettings();
        initial_screen_pass_settings.depth_attachment.store_action = gfx::RenderPass::Attachment::StoreAction::Store;
        frame.initial_screen_pass_ptr = gfx::RenderPass::Create(context, initial_screen_pass_settings);

        // Create final screen pass for sky-box and planet rendering
        gfx::RenderPass::Settings final_screen_pass_settings = frame.screen_pass_ptr->GetSettings();
        final_screen_pass_settings.color_attachments[0].load_action = gfx::RenderPass::Attachment::LoadAction::Load;
        final_screen_pass_settings.depth_attachment.load_action     = gfx::RenderPass::Attachment::LoadAction::Load;
        frame.final_screen_pass_ptr = gfx::RenderPass::Create(context, final_screen_pass_settings);

        // Create parallel command list for asteroids rendering
        frame.parallel_cmd_list_ptr = gfx::ParallelRenderCommandList::Create(context.GetRenderCommandQueue(), *frame.initial_screen_pass_ptr);
        frame.parallel_cmd_list_ptr->SetParallelCommandListsCount(std::thread::hardware_concurrency());
        frame.parallel_cmd_list_ptr->SetName(IndexedName("Parallel Rendering", frame.index));
        frame.parallel_cmd_list_ptr->SetValidationEnabled(false);

        // Create serial command list for asteroids rendering
        frame.serial_cmd_list_ptr = gfx::RenderCommandList::Create(context.GetRenderCommandQueue(), *frame.initial_screen_pass_ptr);
        frame.serial_cmd_list_ptr->SetName(IndexedName("Serial Rendering", frame.index));
        frame.serial_cmd_list_ptr->SetValidationEnabled(false);

        // Create final command list for sky-box and planet rendering
        frame.final_cmd_list_ptr = gfx::RenderCommandList::Create(context.GetRenderCommandQueue(), *frame.final_screen_pass_ptr);
        frame.final_cmd_list_ptr->SetName(IndexedName("Final Rendering", frame.index));
        frame.final_cmd_list_ptr->SetValidationEnabled(false);

        // Rendering command lists sequence
        frame.execute_cmd_list_set_ptr = CreateExecuteCommandListSet(frame);

        // Create uniforms buffer with volatile parameters for the whole scene rendering
        frame.scene_uniforms_buffer_ptr = gfx::Buffer::CreateVolatileBuffer(context, scene_uniforms_data_size);
        frame.scene_uniforms_buffer_ptr->SetName(IndexedName("Scene Uniforms Buffer", frame.index));

        // Create uniforms buffer for Sky-Box rendering
        frame.skybox.uniforms_buffer_ptr = gfx::Buffer::CreateVolatileBuffer(context, sizeof(gfx::SkyBox::Uniforms));
        frame.skybox.uniforms_buffer_ptr->SetName(IndexedName("Sky-box Uniforms Buffer", frame.index));

        // Create uniforms buffer for Planet rendering
        frame.planet.uniforms_buffer_ptr = gfx::Buffer::CreateVolatileBuffer(context, sizeof(Planet::Uniforms));
        frame.planet.uniforms_buffer_ptr->SetName(IndexedName("Planet Uniforms Buffer", frame.index));

        // Create uniforms buffer for Asteroids array rendering
        frame.asteroids.uniforms_buffer_ptr = gfx::Buffer::CreateVolatileBuffer(context, asteroid_uniforms_data_size, true);
        frame.asteroids.uniforms_buffer_ptr->SetName(IndexedName("Asteroids Array Uniforms Buffer", frame.index));

        // Resource bindings for Sky-Box rendering
        frame.skybox.program_bindings_per_instance.resize(1);
        frame.skybox.program_bindings_per_instance[0] = m_sky_box_ptr->CreateProgramBindings(frame.skybox.uniforms_buffer_ptr);

        // Resource bindings for Planet rendering
        frame.planet.program_bindings_per_instance.resize(1);
        frame.planet.program_bindings_per_instance[0] = m_planet_ptr->CreateProgramBindings(m_const_buffer_ptr, frame.planet.uniforms_buffer_ptr);

        // Resource bindings for Asteroids rendering
        frame.asteroids.program_bindings_per_instance = m_asteroids_array_ptr->CreateProgramBindings(m_const_buffer_ptr, frame.scene_uniforms_buffer_ptr, frame.asteroids.uniforms_buffer_ptr);
    }

    CompleteInitialization();
    META_LOG(GetParametersString());
}

bool AsteroidsApp::Resize(const gfx::FrameSize& frame_size, bool is_minimized)
{
    META_FUNCTION_TASK();

    // Resize screen color and depth textures
    if (!UserInterfaceApp::Resize(frame_size, is_minimized))
        return false;
    
    // Update frame buffer and depth textures in initial & final render passes
    for (AsteroidsFrame& frame : GetFrames())
    {
        assert(!!frame.initial_screen_pass_ptr);
        gfx::RenderPass::Settings initial_pass_settings         = frame.initial_screen_pass_ptr->GetSettings();
        initial_pass_settings.color_attachments[0].texture_ptr = frame.screen_texture_ptr;
        initial_pass_settings.depth_attachment.texture_ptr     = GetDepthTexturePtr();
        frame.initial_screen_pass_ptr->Update(initial_pass_settings);
        
        assert(!!frame.final_screen_pass_ptr);
        gfx::RenderPass::Settings final_pass_settings           = frame.final_screen_pass_ptr->GetSettings();
        final_pass_settings.color_attachments[0].texture_ptr = frame.screen_texture_ptr;
        final_pass_settings.depth_attachment.texture_ptr     = GetDepthTexturePtr();
        frame.final_screen_pass_ptr->Update(final_pass_settings);
    }

    m_view_camera.Resize({ static_cast<float>(frame_size.width), static_cast<float>(frame_size.height) });

    return true;
}

bool AsteroidsApp::Update()
{
    META_FUNCTION_TASK();
    META_SCOPE_TIMER("AsteroidsApp::Update");

    if (!UserInterfaceApp::Update())
        return false;

    // Update scene uniforms
    m_scene_uniforms.eye_position    = gfx::Vector4f(m_view_camera.GetOrientation().eye, 1.F);
    m_scene_uniforms.light_position  = m_light_camera.GetOrientation().eye;

    m_sky_box_ptr->Update();
    return true;
}

bool AsteroidsApp::Animate(double elapsed_seconds, double delta_seconds)
{
    META_FUNCTION_TASK();
    bool update_result = m_planet_ptr->Update(elapsed_seconds, delta_seconds);
    update_result     |= m_asteroids_array_ptr->Update(elapsed_seconds, delta_seconds);
    return update_result;
}

bool AsteroidsApp::Render()
{
    META_FUNCTION_TASK();
    META_SCOPE_TIMER("AsteroidsApp::Render");
    if (!UserInterfaceApp::Render())
        return false;

    // Upload uniform buffers to GPU
    AsteroidsFrame& frame = GetCurrentFrame();
    frame.scene_uniforms_buffer_ptr->SetData(m_scene_uniforms_subresources);

    // Asteroids rendering in parallel or in main thread
    if (m_is_parallel_rendering_enabled)
    {
        GetAsteroidsArray().DrawParallel(*frame.parallel_cmd_list_ptr, frame.asteroids, GetViewState());
        frame.parallel_cmd_list_ptr->Commit();
    }
    else
    {
        GetAsteroidsArray().Draw(*frame.serial_cmd_list_ptr, frame.asteroids, GetViewState());
        frame.serial_cmd_list_ptr->Commit();
    }
    
    // Draw planet and sky-box after asteroids to minimize pixel overdraw
    m_planet_ptr->Draw(*frame.final_cmd_list_ptr, frame.planet, GetViewState());
    m_sky_box_ptr->Draw(*frame.final_cmd_list_ptr, frame.skybox, GetViewState());
    RenderOverlay(*frame.final_cmd_list_ptr);
    frame.final_cmd_list_ptr->Commit();

    // Execute rendering commands and present frame to screen
    GetRenderContext().GetRenderCommandQueue().Execute(*frame.execute_cmd_list_set_ptr);
    GetRenderContext().Present();

    return true;
}

void AsteroidsApp::OnContextReleased(gfx::Context& context)
{
    META_FUNCTION_TASK();
    META_SCOPE_TIMERS_FLUSH();

    if (m_asteroids_array_ptr)
    {
        m_asteroids_array_state_ptr = m_asteroids_array_ptr->GetState();
    }

    m_sky_box_ptr.reset();
    m_planet_ptr.reset();
    m_asteroids_array_ptr.reset();
    m_const_buffer_ptr.reset();

    UserInterfaceApp::OnContextReleased(context);
}

void AsteroidsApp::SetAsteroidsComplexity(uint32_t asteroids_complexity)
{
    META_FUNCTION_TASK();

    asteroids_complexity = std::min(g_max_complexity, asteroids_complexity);
    if (m_asteroids_complexity == asteroids_complexity)
        return;

    if (IsRenderContextInitialized())
    {
        GetRenderContext().WaitForGpu(gfx::RenderContext::WaitFor::RenderComplete);
    }

    m_asteroids_complexity = asteroids_complexity;

    const MutableParameters& mutable_parameters         = GetMutableParameters(m_asteroids_complexity);
    m_asteroids_array_settings.instance_count           = mutable_parameters.instances_count;
    m_asteroids_array_settings.unique_mesh_count        = mutable_parameters.unique_mesh_count;
    m_asteroids_array_settings.textures_count           = mutable_parameters.textures_count;
    m_asteroids_array_settings.min_asteroid_scale_ratio = mutable_parameters.scale_ratio / 10.F;
    m_asteroids_array_settings.max_asteroid_scale_ratio = mutable_parameters.scale_ratio;

    m_asteroids_array_ptr.reset();
    m_asteroids_array_state_ptr.reset();

    if (IsRenderContextInitialized())
        GetRenderContext().Reset();

    UpdateParametersText();
}

void AsteroidsApp::SetParallelRenderingEnabled(bool is_parallel_rendering_enabled)
{
    META_FUNCTION_TASK();
    if (m_is_parallel_rendering_enabled == is_parallel_rendering_enabled)
        return;

    META_SCOPE_TIMERS_FLUSH();
    m_is_parallel_rendering_enabled = is_parallel_rendering_enabled;
    for(AsteroidsFrame& frame : GetFrames())
    {
        frame.execute_cmd_list_set_ptr = CreateExecuteCommandListSet(frame);
    }

    UpdateParametersText();
    META_LOG(GetParametersString());
}

AsteroidsArray& AsteroidsApp::GetAsteroidsArray() const
{
    META_FUNCTION_TASK();
    assert(!!m_asteroids_array_ptr);
    return *m_asteroids_array_ptr;
}

std::string AsteroidsApp::GetParametersString()
{
    META_FUNCTION_TASK();

    std::stringstream ss;
    ss << "Asteroids simulation parameters:"
       << std::endl << "  - simulation complexity [0.."  << g_max_complexity << "]: " << m_asteroids_complexity
       << std::endl << "  - asteroid instances count:     " << m_asteroids_array_settings.instance_count
       << std::endl << "  - unique meshes count:          " << m_asteroids_array_settings.unique_mesh_count
       << std::endl << "  - mesh subdivisions count:      " << m_asteroids_array_settings.subdivisions_count
       << std::endl << "  - unique textures count:        " << m_asteroids_array_settings.textures_count << " "
       << std::endl << "  - asteroid textures size:       " << static_cast<std::string>(m_asteroids_array_settings.texture_dimensions)
       << std::endl << "  - textures array binding:       " << (m_asteroids_array_settings.textures_array_enabled ? "ON" : "OFF")
       << std::endl << "  - parallel rendering:           " << (m_is_parallel_rendering_enabled ? "ON" : "OFF")
       << std::endl << "  - asteroid animations:          " << (!GetAnimations().IsPaused() ? "ON" : "OFF")
       << std::endl << "  - CPU h/w thread count:         " << std::thread::hardware_concurrency();

    return ss.str();
}

Ptr<gfx::CommandListSet> AsteroidsApp::CreateExecuteCommandListSet(AsteroidsFrame& frame)
{
    return gfx::CommandListSet::Create({
        m_is_parallel_rendering_enabled
            ? static_cast<gfx::CommandList&>(*frame.parallel_cmd_list_ptr)
            : static_cast<gfx::CommandList&>(*frame.serial_cmd_list_ptr),
        *frame.final_cmd_list_ptr
    });
}

} // namespace Methane::Samples

int main(int argc, const char* argv[])
{
    return Methane::Samples::AsteroidsApp().Run({ argc, argv });
}
