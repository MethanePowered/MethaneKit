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
#include <magic_enum.hpp>

namespace Methane::Graphics
{

static constexpr double g_title_update_interval_sec = 1.0;

IApp::Settings& IApp::Settings::SetScreenPassAccess(RenderPass::Access new_screen_pass_access) noexcept
{
    META_FUNCTION_TASK();
    screen_pass_access = new_screen_pass_access;
    return *this;
}

IApp::Settings& IApp::Settings::SetAnimationsEnabled(bool new_animations_enabled) noexcept
{
    META_FUNCTION_TASK();
    animations_enabled = new_animations_enabled;
    return *this;
}

IApp::Settings& IApp::Settings::SetShowHudInWindowTitle(bool new_show_hud_in_window_title) noexcept
{
    META_FUNCTION_TASK();
    show_hud_in_window_title = new_show_hud_in_window_title;
    return *this;
}

IApp::Settings& IApp::Settings::SetDefaultDeviceIndex(int32_t new_default_device_index) noexcept
{
    META_FUNCTION_TASK();
    default_device_index = new_default_device_index;
    return *this;
}

IApp::Settings& IApp::Settings::SetDeviceCapabilities(Device::Capabilities&& new_device_capabilities) noexcept
{
    META_FUNCTION_TASK();
    device_capabilities = std::move(new_device_capabilities);
    return *this;
}

AppSettings& AppSettings::SetPlatformAppSettings(Platform::IApp::Settings&& new_platform_app_settings) noexcept
{
    META_FUNCTION_TASK();
    platform_app = std::move(new_platform_app_settings);
    return *this;
}

AppSettings& AppSettings::SetGraphicsAppSettings(Graphics::IApp::Settings&& new_graphics_app_settings) noexcept
{
    META_FUNCTION_TASK();
    graphics_app = std::move(new_graphics_app_settings);
    return *this;
}

AppSettings& AppSettings::SetRenderContextSettings(RenderContext::Settings&& new_render_context_settings) noexcept
{
    META_FUNCTION_TASK();
    render_context = std::move(new_render_context_settings);
    return *this;
}

AppBase::AppBase(const AppSettings& settings, Data::Provider& textures_provider)
    : Platform::App(settings.platform_app)
    , m_settings(settings.graphics_app)
    , m_initial_context_settings(settings.render_context)
    , m_image_loader(textures_provider)
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;

    add_option("-a,--animations", m_settings.animations_enabled, "Enable animations");
    add_option("-d,--device", m_settings.default_device_index, "Render at adapter index, use -1 for software adapter");
    add_option("-v,--vsync", m_initial_context_settings.vsync_enabled, "Vertical synchronization");
    add_option("-b,--frame-buffers", m_initial_context_settings.frame_buffers_count, "Frame buffers count in swap-chain");

#ifdef _WIN32
    add_flag("-e,--emulated-render-pass",
             [this](int64_t is_emulated) { if (is_emulated) m_initial_context_settings.options_mask |= Context::Options::EmulatedRenderPassOnWindows; },
             "Render pass emulation with traditional DX API");
    add_flag("-q,--blit-with-direct-queue",
             [this](int64_t is_direct) { if (is_direct) m_initial_context_settings.options_mask |= Context::Options::BlitWithDirectQueueOnWindows; },
             "Blit command lists and queues use DIRECT instead of COPY type in DX API");
#endif
}

AppBase::~AppBase()
{
    META_FUNCTION_TASK();
    if (m_context_ptr)
    {
        // Prevent OnContextReleased callback emitting during application destruction
        static_cast<Data::IEmitter<IContextCallback>&>(*m_context_ptr).Disconnect(*this);
    }
}

void AppBase::InitContext(const Platform::AppEnvironment& env, const FrameSize& frame_size)
{
    META_FUNCTION_TASK();
    META_LOG("\n====================== CONTEXT INITIALIZATION ======================");

    const Ptrs<Device>& devices = System::Get().UpdateGpuDevices(env, m_settings.device_capabilities);
    META_CHECK_ARG_NOT_EMPTY_DESCR(devices, "no suitable GPU devices were found for application rendering");

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
    m_context_ptr->SetName("Graphics Context");
    static_cast<Data::IEmitter<IContextCallback>&>(*m_context_ptr).Connect(*this);

    // Fill initial screen render-pass pattern settings
    m_screen_pass_pattern_settings.shader_access_mask = m_settings.screen_pass_access;
    m_screen_pass_pattern_settings.is_final_pass      = true;

    // Final frame color attachment
    Data::Index attachment_index = 0U;
    m_screen_pass_pattern_settings.color_attachments = {
        RenderPass::ColorAttachment(
            attachment_index++,
            m_initial_context_settings.color_format, 1U,
            m_initial_context_settings.clear_color.has_value()
            ? RenderPass::Attachment::LoadAction::Clear
            : RenderPass::Attachment::LoadAction::DontCare,
            RenderPass::Attachment::StoreAction::Store,
            m_initial_context_settings.clear_color.value_or(Color4F())
        )
    };

    // Create frame depth texture and attachment description
    if (m_initial_context_settings.depth_stencil_format != PixelFormat::Unknown)
    {
        static constexpr DepthStencil s_default_depth_stencil{ Depth(1.F), Stencil(0) };
        m_screen_pass_pattern_settings.depth_attachment = RenderPass::DepthAttachment(
            attachment_index++,
            m_initial_context_settings.depth_stencil_format, 1U,
            m_initial_context_settings.clear_depth_stencil.has_value()
            ? RenderPass::Attachment::LoadAction::Clear
            : RenderPass::Attachment::LoadAction::DontCare,
            RenderPass::Attachment::StoreAction::DontCare,
            m_initial_context_settings.clear_depth_stencil.value_or(s_default_depth_stencil).first
        );
    }

    AddInputControllers({ std::make_shared<AppContextController>(*m_context_ptr) });

    SetFullScreen(m_initial_context_settings.is_full_screen);
}

void AppBase::Init()
{
    META_FUNCTION_TASK();
    META_LOG("\n======================== APP INITIALIZATION ========================");

    if (!m_settings.animations_enabled)
    {
        m_settings.animations_enabled = true;
        SetBaseAnimationsEnabled(false);
    }

    META_CHECK_ARG_NOT_NULL(m_context_ptr);
    const RenderContext::Settings& context_settings = m_context_ptr->GetSettings();

    // Create frame depth texture and attachment description
    if (context_settings.depth_stencil_format != PixelFormat::Unknown)
    {
        m_depth_texture_ptr = Texture::CreateDepthStencilBuffer(*m_context_ptr);
        m_depth_texture_ptr->SetName("Depth Texture");
    }

    // Create screen render pass pattern
    m_screen_render_pattern_ptr = RenderPattern::Create(*m_context_ptr, m_screen_pass_pattern_settings);
    m_screen_render_pattern_ptr->SetName("Final Render Pass");

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

    META_LOG("\n========================== FRAMES RESIZING ==========================");

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

    META_LOG("\n========================== FRAME {} UPDATING =========================", m_context_ptr ? m_context_ptr->GetFrameIndex() : 0U);

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
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // NOSONAR - false positive
        return false;
    }

    META_CHECK_ARG_NOT_NULL_DESCR(m_context_ptr, "RenderContext is not initialized before rendering.");
    if (!m_context_ptr->ReadyToRender())
        return false;

    META_LOG("\n========================= FRAME {} RENDERING =========================", m_context_ptr->GetFrameIndex());

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

Texture::Views AppBase::GetScreenPassAttachments(Texture& frame_buffer_texture) const
{
    META_FUNCTION_TASK();
    Texture::Views attachments{
        Texture::View(frame_buffer_texture)
    };

    if (m_depth_texture_ptr)
        attachments.emplace_back(*m_depth_texture_ptr);

    return attachments;
}

Ptr<RenderPass> AppBase::CreateScreenRenderPass(Texture& frame_buffer_texture) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_context_ptr);
    return RenderPass::Create(GetScreenRenderPattern(), {
        GetScreenPassAttachments(frame_buffer_texture),
        m_context_ptr->GetSettings().frame_size
    });
}

Opt<AppBase::ResourceRestoreInfo> AppBase::ReleaseDepthTexture()
{
    META_FUNCTION_TASK();
    if (!m_depth_texture_ptr)
        return std::nullopt;

    ResourceRestoreInfo depth_restore_info(*m_depth_texture_ptr);
    m_depth_texture_ptr.reset();
    return depth_restore_info;
}

void AppBase::RestoreDepthTexture(const Opt<ResourceRestoreInfo>& depth_restore_info_opt)
{
    META_FUNCTION_TASK();
    if (!depth_restore_info_opt)
        return;

    m_depth_texture_ptr = Texture::CreateDepthStencilBuffer(GetRenderContext());
    m_depth_texture_ptr->RestoreDescriptorViews(depth_restore_info_opt->descriptor_by_view_id);
    m_depth_texture_ptr->SetName(depth_restore_info_opt->name);
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
    const std::string title = fmt::format("{:s}        {:d} FPS, {:.2f} ms, {:.2f}% CPU |  {:d} x {:d}  |  {:d} FB  |  VSync {:s}  |  {:s}  |  {:s}  |  F1 - help",
                                          GetPlatformAppSettings().name,
                                          average_fps, average_frame_timing.GetTotalTimeMSec(), average_frame_timing.GetCpuTimePercent(),
                                          context_settings.frame_size.GetWidth(), context_settings.frame_size.GetHeight(),
                                          context_settings.frame_buffers_count, (context_settings.vsync_enabled ? "ON" : "OFF"),
                                          m_context_ptr->GetDevice().GetAdapterName(),
                                          magic_enum::enum_name(System::GetGraphicsApi()));

    SetWindowTitle(title);
}

void AppBase::CompleteInitialization() const
{
    META_FUNCTION_TASK();
    if (m_context_ptr)
    {
        m_context_ptr->CompleteInitialization();
    }
}

void AppBase::WaitForRenderComplete() const
{
    META_FUNCTION_TASK();
    if (m_context_ptr)
    {
        m_context_ptr->WaitForGpu(Context::WaitFor::RenderComplete);
    }
}

void AppBase::OnContextReleased(Context&)
{
    META_FUNCTION_TASK();

    m_restore_animations_enabled = m_settings.animations_enabled;
    SetBaseAnimationsEnabled(false);

    m_screen_render_pattern_ptr.reset();
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