/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/AppBase.h
Base implementation of the Methane graphics application.

******************************************************************************/

#include <Methane/Graphics/AppBase.h>
#include <Methane/Graphics/AppCameraController.h>
#include <Methane/Graphics/AppContextController.h>
#include <Methane/Graphics/Device.h>
#include <Methane/Graphics/Texture.h>
#include <Methane/Graphics/RenderState.h>
#include <Methane/Graphics/RenderPass.h>
#include <Methane/Graphics/FpsCounter.h>
#include <Methane/Data/Provider.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <fmt/format.h>

namespace Methane::Graphics
{

static constexpr double g_title_update_interval_sec = 1.0;

AppBase::AppBase(const AppSettings& settings, Data::Provider& textures_provider)
    : Platform::App(settings.platform_app)
    , m_settings(settings.graphics_app)
    , m_initial_context_settings(settings.render_context)
    , m_image_loader(textures_provider)
{
    META_FUNCTION_TASK();
    add_option("-a,--animations", m_settings.animations_enabled, "Enable animations", true);
    add_option("-d,--device", m_settings.default_device_index, "Render at adapter index, use -1 for software adapter", true);
    add_option("-v,--vsync", m_initial_context_settings.vsync_enabled, "Vertical synchronization", true);
    add_option("-b,--frame-buffers", m_initial_context_settings.frame_buffers_count, "Frame buffers count in swap-chain", true);
    add_flag("-e,--emulated-render-pass", m_initial_context_settings.is_emulated_render_pass, "Render pass emulation on Windows");
}

void AppBase::InitContext(const Platform::AppEnvironment& env, const FrameSize& frame_size)
{
    META_FUNCTION_TASK();
    const Ptrs<Device>& devices = System::Get().UpdateGpuDevices();
    META_CHECK_ARG_NOT_EMPTY(devices);

    Ptr<Device> device_ptr;
    if (m_settings.default_device_index < 0)
        device_ptr = System::Get().GetSoftwareGpuDevice();
    else
        device_ptr = static_cast<size_t>(m_settings.default_device_index) < devices.size()
                   ? devices[m_settings.default_device_index]
                   : devices.front();
    META_CHECK_ARG_NOT_NULL(device_ptr);

    // Create render context of the current window size
    m_initial_context_settings.frame_size = frame_size;
    m_context_ptr = RenderContext::Create(env, *device_ptr, GetParallelExecutor(), m_initial_context_settings);
    m_context_ptr->SetName("App Render Context");
    m_context_ptr->Connect(*this);

    AddInputControllers({ std::make_shared<AppContextController>(*m_context_ptr) });

    SetFullScreen(m_initial_context_settings.is_full_screen);
}

void AppBase::Init()
{
    META_FUNCTION_TASK();

    if (!m_settings.animations_enabled)
    {
        m_settings.animations_enabled = true;
        SetBaseAnimationsEnabled(false);
    }

    META_CHECK_ARG_NOT_NULL(m_context_ptr);
    const RenderContext::Settings& context_settings = m_context_ptr->GetSettings();

    // Create depth texture for FB rendering
    if (context_settings.depth_stencil_format != PixelFormat::Unknown)
    {
        m_depth_texture_ptr = Texture::CreateDepthStencilBuffer(*m_context_ptr);
        m_depth_texture_ptr->SetName("Depth Texture");
    }

    m_view_state_ptr = ViewState::Create({
         { GetFrameViewport(context_settings.frame_size)    },
         { GetFrameScissorRect(context_settings.frame_size) }
     });

    Platform::App::Init();
}

void AppBase::StartResizing()
{
    META_FUNCTION_TASK();
    Platform::App::StartResizing();
    m_restore_animations_enabled = m_settings.animations_enabled;
    SetBaseAnimationsEnabled(false);
}

void AppBase::EndResizing()
{
    META_FUNCTION_TASK();
    SetBaseAnimationsEnabled(m_restore_animations_enabled);
    Platform::App::EndResizing();
}

bool AppBase::Resize(const FrameSize& frame_size, bool is_minimized)
{
    META_FUNCTION_TASK();
    if (!Platform::App::Resize(frame_size, is_minimized))
        return false;

    m_initial_context_settings.frame_size = frame_size;

    // Update viewports and scissor rects state
    m_view_state_ptr->SetViewports({ GetFrameViewport(frame_size) });
    m_view_state_ptr->SetScissorRects({ GetFrameScissorRect(frame_size) });

    return true;
}

bool AppBase::Update()
{
    META_FUNCTION_TASK();
    if (Platform::App::IsMinimized())
        return false;

    System::Get().CheckForChanges();

    // Update HUD info in window title
    if (m_settings.show_hud_in_window_title &&
        m_title_update_timer.GetElapsedSecondsD() >= g_title_update_interval_sec)
    {
        UpdateWindowTitle();
        m_title_update_timer.Reset();
    }

    GetAnimations().Update();
    return true;
}

bool AppBase::Render()
{
    META_FUNCTION_TASK();
    if (Platform::App::IsMinimized())
    {
        // No need to render frames while window is minimized.
        // Sleep thread for a while to not heat CPU by running the message loop
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return false;
    }

    META_CHECK_ARG_NOT_NULL_DESCR(m_context_ptr, "RenderContext is not initialized before rendering.");
    if (!m_context_ptr->ReadyToRender())
        return false;

    // Wait for previous frame rendering is completed and switch to next frame
    m_context_ptr->WaitForGpu(Context::WaitFor::FramePresented);
    return true;
}

bool AppBase::SetFullScreen(bool is_full_screen)
{
    META_FUNCTION_TASK();
    if (m_context_ptr)
        m_context_ptr->SetFullScreen(is_full_screen);

    return Platform::App::SetFullScreen(is_full_screen);
}

// Graphics::IApp interface

bool AppBase::SetBaseAnimationsEnabled(bool animations_enabled)
{
    META_FUNCTION_TASK();
    if (m_settings.animations_enabled == animations_enabled)
        return false;

    m_settings.animations_enabled = animations_enabled;

    // Pause animations or resume from the paused state
    if (m_settings.animations_enabled)
        GetAnimations().Resume();
    else
        GetAnimations().Pause();

    // Disable all camera controllers while animations are paused, since they can not function without animations
    Refs<AppCameraController> camera_controllers = GetInputState().template GetControllersOfType<AppCameraController>();
    for(const Ref<AppCameraController>& camera_controller : camera_controllers)
    {
        camera_controller.get().SetEnabled(animations_enabled);
    }

    return true;
}

void AppBase::SetShowHudInWindowTitle(bool show_hud_in_window_title)
{
    META_FUNCTION_TASK();
    if (m_settings.show_hud_in_window_title == show_hud_in_window_title)
        return;

    m_settings.show_hud_in_window_title = show_hud_in_window_title;
    UpdateWindowTitle();
}

Ptr<RenderPass> AppBase::CreateScreenRenderPass(const Ptr<Texture>& frame_buffer_texture) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(frame_buffer_texture);

    const RenderContext::Settings& context_settings = m_context_ptr->GetSettings();
    static constexpr DepthStencil s_default_depth_stencil{ Depth(1.F), Stencil(0) };

    return RenderPass::Create(*m_context_ptr, {
        {
            RenderPass::ColorAttachment(
                {
                    Texture::Location{ frame_buffer_texture },
                    context_settings.clear_color.has_value()
                        ? RenderPass::Attachment::LoadAction::Clear
                        : RenderPass::Attachment::LoadAction::DontCare,
                    RenderPass::Attachment::StoreAction::Store,
                },
                context_settings.clear_color.value_or(Color4F())
            )
        },
        RenderPass::DepthAttachment(
            {
                Texture::Location{ m_depth_texture_ptr },
                context_settings.clear_depth_stencil.has_value()
                    ? RenderPass::Attachment::LoadAction::Clear
                    : RenderPass::Attachment::LoadAction::DontCare,
                RenderPass::Attachment::StoreAction::DontCare,
            },
            context_settings.clear_depth_stencil.value_or(s_default_depth_stencil).first
        ),
        RenderPass::StencilAttachment(),
        m_settings.screen_pass_access,
        true // final render pass
    });
}

AppBase::ResourceRestoreInfo AppBase::ReleaseDepthTexture()
{
    META_FUNCTION_TASK();
    ResourceRestoreInfo depth_restore_info(m_depth_texture_ptr);
    m_depth_texture_ptr.reset();
    return depth_restore_info;
}

void AppBase::RestoreDepthTexture(const ResourceRestoreInfo& depth_restore_info)
{
    META_FUNCTION_TASK();
    if (depth_restore_info.descriptor_by_usage.empty())
        return;

    m_depth_texture_ptr = Texture::CreateDepthStencilBuffer(GetRenderContext(), depth_restore_info.descriptor_by_usage);
    m_depth_texture_ptr->SetName(depth_restore_info.name);
}

void AppBase::UpdateWindowTitle()
{
    META_FUNCTION_TASK();
    if (!m_settings.show_hud_in_window_title)
    {
        SetWindowTitle(GetPlatformAppSettings().name);
        return;
    }

    META_CHECK_ARG_NOT_NULL(m_context_ptr);
    const RenderContext::Settings& context_settings      = m_context_ptr->GetSettings();
    const FpsCounter&              fps_counter           = m_context_ptr->GetFpsCounter();
    const uint32_t                 average_fps           = fps_counter.GetFramesPerSecond();
    const FpsCounter::FrameTiming  average_frame_timing  = fps_counter.GetAverageFrameTiming();
    const std::string title = fmt::format("{:s}        {:d} FPS, {:.2f} ms, {:.2f}% CPU |  {:d} x {:d}  |  {:d} FB  |  VSync {:s}  |  {:s}  |  F1 - help",
                                          GetPlatformAppSettings().name,
                                          average_fps, average_frame_timing.GetTotalTimeMSec(), average_frame_timing.GetCpuTimePercent(),
                                          context_settings.frame_size.GetWidth(), context_settings.frame_size.GetHeight(),
                                          context_settings.frame_buffers_count, (context_settings.vsync_enabled ? "ON" : "OFF"),
                                          m_context_ptr->GetDevice().GetAdapterName());

    SetWindowTitle(title);
}

void AppBase::CompleteInitialization() const
{
    m_context_ptr->CompleteInitialization();
}

void AppBase::OnContextReleased(Context&)
{
    META_FUNCTION_TASK();

    m_restore_animations_enabled = m_settings.animations_enabled;
    SetBaseAnimationsEnabled(false);

    m_depth_texture_ptr.reset();
    m_view_state_ptr.reset();

    Deinitialize();
}

void AppBase::OnContextInitialized(Context&)
{
    META_FUNCTION_TASK();
    Init();
    SetBaseAnimationsEnabled(m_restore_animations_enabled);
}

std::string AppBase::IndexedName(const std::string& base_name, uint32_t index)
{
    META_FUNCTION_TASK();
    return fmt::format("{} {}", base_name, index);
}

} // namespace Methane::Graphics