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
#include <Methane/Graphics/RHI/System.h>
#include <Methane/Graphics/RHI/RenderState.h>
#include <Methane/Data/IProvider.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <fmt/format.h>
#include <magic_enum.hpp>
#include <thread>

namespace Methane::Graphics
{

static constexpr double g_title_update_interval_sec = 1.0;

AppBase::ResourceRestoreInfo::ResourceRestoreInfo(const Rhi::IResource& resource)
    : descriptor_by_view_id(resource.GetDescriptorByViewId())
    , name(resource.GetName())
{ }

AppBase::AppBase(const CombinedAppSettings& settings, Data::IProvider& textures_provider)
    : Platform::App(settings.platform_app)
    , m_settings(settings.graphics_app)
    , m_initial_context_settings(settings.render_context)
    , m_image_loader(textures_provider)
{
    META_FUNCTION_TASK();

    add_option("-a,--animations", m_settings.animations_enabled, "Enable animations");
    add_option("-d,--device", m_settings.default_device_index, "Render at adapter index, use -1 for software adapter");
    add_option("-v,--vsync", m_initial_context_settings.vsync_enabled, "Vertical synchronization");
    add_option("-b,--frame-buffers", m_initial_context_settings.frame_buffers_count, "Frame buffers count in swap-chain");

#ifdef _WIN32
    add_flag("-e,--emulated-render-pass",
             [this](int64_t is_emulated) { m_initial_context_settings.options_mask.SetBit(Rhi::ContextOption::EmulateD3D12RenderPass, is_emulated); },
             "Render pass emulation with traditional DX API");
    add_flag("-q,--transfer-with-direct-queue",
             [this](int64_t is_direct) { m_initial_context_settings.options_mask.SetBit(Rhi::ContextOption::TransferWithD3D12DirectQueue, is_direct); },
             "Transfer command lists and queues use DIRECT instead of COPY type in DX API");
#endif
}

AppBase::~AppBase()
{
    META_FUNCTION_TASK();
    if (m_context.IsInitialized())
    {
        // Prevent OnContextReleased callback emitting during application destruction
        m_context.Disconnect(*this);
    }
}

Rhi::Device AppBase::GetDefaultDevice() const
{
    META_FUNCTION_TASK();
    if (m_settings.default_device_index < 0)
        return Rhi::System::Get().GetSoftwareGpuDevice();

    const std::vector<Rhi::Device>& devices = Rhi::System::Get().GetGpuDevices();
    META_CHECK_ARG_NOT_EMPTY_DESCR(devices, "no suitable GPU devices were found for application rendering");

    if (static_cast<size_t>(m_settings.default_device_index) < devices.size())
        return devices[m_settings.default_device_index];

    return devices.front();
}

void AppBase::InitContext(const Platform::AppEnvironment& env, const FrameSize& frame_size)
{
    META_FUNCTION_TASK();
    META_LOG("\n====================== CONTEXT INITIALIZATION ======================");

    // Get default device for rendering
    Rhi::System::Get().UpdateGpuDevices(env, m_settings.device_capabilities);
    const Rhi::Device device = GetDefaultDevice();
    META_CHECK_ARG_TRUE(device.IsInitialized());

    // Create render context of the current window size
    m_initial_context_settings.frame_size = frame_size;
    m_context = device.CreateRenderContext(env, GetParallelExecutor(), m_initial_context_settings);
    m_context.SetName("Graphics Context");
    m_context.Connect(*this);

    // Fill initial screen render-pass pattern settings
    m_screen_pass_pattern_settings.shader_access = m_settings.screen_pass_access;
    m_screen_pass_pattern_settings.is_final_pass = true;

    // Final frame color attachment
    Data::Index attachment_index = 0U;
    m_screen_pass_pattern_settings.color_attachments = {
        Rhi::IRenderPass::ColorAttachment(
            attachment_index++,
            m_initial_context_settings.color_format, 1U,
            m_initial_context_settings.clear_color.has_value()
                ? Rhi::IRenderPass::Attachment::LoadAction::Clear
                : Rhi::IRenderPass::Attachment::LoadAction::DontCare,
            Rhi::IRenderPass::Attachment::StoreAction::Store,
            m_initial_context_settings.clear_color.value_or(Color4F())
        )
    };

    // Create frame depth texture and attachment description
    if (m_initial_context_settings.depth_stencil_format != PixelFormat::Unknown)
    {
        static constexpr DepthStencilValues s_default_depth_stencil{ Depth(1.F), Stencil(0) };
        m_screen_pass_pattern_settings.depth_attachment = Rhi::IRenderPass::DepthAttachment(
            attachment_index++,
            m_initial_context_settings.depth_stencil_format, 1U,
            m_initial_context_settings.clear_depth_stencil.has_value()
                ? Rhi::IRenderPass::Attachment::LoadAction::Clear
                : Rhi::IRenderPass::Attachment::LoadAction::DontCare,
            Rhi::IRenderPass::Attachment::StoreAction::DontCare,
            m_initial_context_settings.clear_depth_stencil.value_or(s_default_depth_stencil).first
        );
    }

    AddInputControllers({ std::make_shared<AppContextController>(m_context.GetInterface()) });

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

    const Rhi::RenderContextSettings& context_settings = m_context.GetSettings();

    // Create frame depth texture and attachment description
    if (context_settings.depth_stencil_format != PixelFormat::Unknown)
    {
        m_depth_texture = m_context.CreateTexture(Rhi::TextureSettings::ForDepthStencil(m_context.GetSettings()));
        m_depth_texture.SetName("Depth Texture");
    }

    // Create screen render pass pattern
    m_screen_render_pattern = m_context.CreateRenderPattern(m_screen_pass_pattern_settings);
    m_screen_render_pattern.SetName("Final Render Pass");

    m_view_state = Rhi::ViewState({
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
    m_view_state.SetViewports({ GetFrameViewport(frame_size) });
    m_view_state.SetScissorRects({ GetFrameScissorRect(frame_size) });

    return true;
}

bool AppBase::Update()
{
    META_FUNCTION_TASK();
    if (Platform::App::IsMinimized())
        return false;

    META_LOG("\n========================== FRAME {} UPDATING =========================",
             m_context.IsInitialized() ? m_context.GetFrameIndex() : 0U);

    Rhi::ISystem::Get().CheckForChanges();

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

    META_CHECK_ARG_TRUE_DESCR(m_context.IsInitialized(), "RenderContext is not initialized before rendering.");
    if (!m_context.ReadyToRender())
        return false;

    META_LOG("\n========================= FRAME {} RENDERING =========================", m_context.GetFrameIndex());

    // Wait for previous frame rendering is completed and switch to next frame
    m_context.WaitForGpu(Rhi::IContext::WaitFor::FramePresented);
    return true;
}

bool AppBase::SetFullScreen(bool is_full_screen)
{
    META_FUNCTION_TASK();
    if (m_context.IsInitialized())
        m_context.SetFullScreen(is_full_screen);

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

Rhi::TextureViews AppBase::GetScreenPassAttachments(const Rhi::Texture& frame_buffer_texture) const
{
    META_FUNCTION_TASK();
    Rhi::TextureViews attachments{
        Rhi::TextureView(frame_buffer_texture.GetInterface())
    };

    if (m_depth_texture.IsInitialized())
        attachments.emplace_back(m_depth_texture.GetInterface());

    return attachments;
}

Rhi::RenderPass AppBase::CreateScreenRenderPass(const Rhi::Texture& frame_buffer_texture) const
{
    META_FUNCTION_TASK();
    return Rhi::RenderPass(GetScreenRenderPattern(), {
        GetScreenPassAttachments(frame_buffer_texture),
        m_context.GetSettings().frame_size
    });
}

Opt<AppBase::ResourceRestoreInfo> AppBase::ReleaseDepthTexture()
{
    META_FUNCTION_TASK();
    if (!m_depth_texture.IsInitialized())
        return std::nullopt;

    ResourceRestoreInfo depth_restore_info(m_depth_texture.GetInterface());
    m_depth_texture = {};
    return depth_restore_info;
}

void AppBase::RestoreDepthTexture(const Opt<ResourceRestoreInfo>& depth_restore_info_opt)
{
    META_FUNCTION_TASK();
    if (!depth_restore_info_opt)
        return;

    const Rhi::RenderContext& render_context = GetRenderContext();
    m_depth_texture = render_context.CreateTexture(Rhi::TextureSettings::ForDepthStencil(render_context.GetSettings()));
    m_depth_texture.RestoreDescriptorViews(depth_restore_info_opt->descriptor_by_view_id);
    m_depth_texture.SetName(depth_restore_info_opt->name);
}

void AppBase::UpdateWindowTitle()
{
    META_FUNCTION_TASK();
    if (!m_settings.show_hud_in_window_title || !m_context.IsInitialized())
    {
        SetWindowTitle(GetPlatformAppSettings().name);
        return;
    }

    const Rhi::RenderContextSettings& context_settings = m_context.GetSettings();
    const Data::IFpsCounter&          fps_counter      = m_context.GetFpsCounter();
    const uint32_t                    average_fps      = fps_counter.GetFramesPerSecond();
    const Data::FrameTiming       average_frame_timing = fps_counter.GetAverageFrameTiming();

    const std::string title = fmt::format("{:s}        {:d} FPS, {:.2f} ms, {:.2f}% CPU |  {:d} x {:d}  |  {:d} FB  |  VSync {:s}  |  {:s}  |  {:s}  |  F1 - help",
                                          GetPlatformAppSettings().name,
                                          average_fps, average_frame_timing.GetTotalTimeMSec(), average_frame_timing.GetCpuTimePercent(),
                                          context_settings.frame_size.GetWidth(), context_settings.frame_size.GetHeight(),
                                          context_settings.frame_buffers_count, (context_settings.vsync_enabled ? "ON" : "OFF"),
                                          m_context.GetDevice().GetAdapterName(),
                                          magic_enum::enum_name(Rhi::ISystem::GetNativeApi()));

    SetWindowTitle(title);
}

void AppBase::CompleteInitialization() const
{
    META_FUNCTION_TASK();
    if (m_context.IsInitialized())
    {
        m_context.CompleteInitialization();
    }
}

void AppBase::WaitForRenderComplete() const
{
    META_FUNCTION_TASK();
    if (m_context.IsInitialized())
    {
        m_context.WaitForGpu(Rhi::IContext::WaitFor::RenderComplete);
    }
}

void AppBase::OnContextReleased(Rhi::IContext&)
{
    META_FUNCTION_TASK();

    m_restore_animations_enabled = m_settings.animations_enabled;
    SetBaseAnimationsEnabled(false);

    m_screen_render_pattern = {};
    m_depth_texture = {};
    m_view_state = {};

    Deinitialize();
}

void AppBase::OnContextInitialized(Rhi::IContext&)
{
    META_FUNCTION_TASK();
    Init();
    SetBaseAnimationsEnabled(m_restore_animations_enabled);
}

} // namespace Methane::Graphics
