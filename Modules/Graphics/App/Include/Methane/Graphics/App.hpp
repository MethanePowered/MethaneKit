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

FILE: Methane/Graphics/App.hpp
Base template class of the Methane graphics application with multiple frame buffers.
Base frame class provides frame buffer management with resize handling.

******************************************************************************/

#pragma once

#include "App.h"
#include "AppController.h"
#include "AppCameraController.h"
#include "AppContextController.h"

#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Data/AnimationsPool.h>
#include <Methane/Data/Receiver.hpp>
#include <Methane/Platform/App.h>
#include <Methane/Graphics/Types.h>
#include <Methane/Graphics/Device.h>
#include <Methane/Graphics/RenderContext.h>
#include <Methane/Graphics/Texture.h>
#include <Methane/Graphics/RenderPass.h>
#include <Methane/Graphics/RenderCommandList.h>
#include <Methane/Graphics/FpsCounter.h>
#include <Methane/Graphics/ImageLoader.h>
#include <Methane/Timer.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <fmt/format.h>
#include <vector>
#include <sstream>
#include <memory>
#include <thread>

namespace Methane::Graphics
{

struct AppFrame
{
    const uint32_t  index = 0;
    Ptr<Texture>    screen_texture_ptr;
    Ptr<RenderPass> screen_pass_ptr;

    explicit AppFrame(uint32_t frame_index) : index(frame_index) { META_FUNCTION_TASK(); }

    // AppFrame interface
    virtual void ReleaseScreenPassAttachmentTextures()
    {
        screen_pass_ptr->ReleaseAttachmentTextures();
        screen_texture_ptr.reset();
    }
};

struct AppSettings
{
    Platform::App::Settings platform_app;
    IApp::Settings          graphics_app;
    RenderContext::Settings render_context;
};

template<typename FrameT, typename IAppT = Graphics::IApp, typename = std::enable_if_t<std::is_base_of_v<AppFrame, FrameT>>>
class App
    : public IAppT
    , public Platform::App
    , protected Data::Receiver<IContextCallback>
{
    static_assert(std::is_base_of<AppFrame, FrameT>::value, "Application Frame type must be derived from AppFrame.");

public:
    explicit App(const AppSettings& settings)
        : Platform::App(settings.platform_app)
        , m_settings(settings.graphics_app)
        , m_initial_context_settings(settings.render_context)
        , m_image_loader(Data::TextureProvider::Get())
    {
        META_FUNCTION_TASK();
        add_option("-a,--animations", m_settings.animations_enabled, "Enable animations", true);
        add_option("-d,--device", m_settings.default_device_index, "Render at adapter index, use -1 for software adapter", true);
        add_option("-v,--vsync", m_initial_context_settings.vsync_enabled, "Vertical synchronization", true);
        add_option("-b,--frame-buffers", m_initial_context_settings.frame_buffers_count, "Frame buffers count in swap-chain", true);
        add_flag("-e,--emulated-render-pass", m_initial_context_settings.is_emulated_render_pass, "Render pass emulation on Windows");
    }

    App(const AppSettings& settings, const std::string& help_description)
        : App(settings)
    {
        META_FUNCTION_TASK();
        AddInputControllers({ std::make_shared<AppController>(*this, help_description) });
    }

    App(const App&) = delete;
    App(App&&) = delete;

    ~App() override
    {
        // WARNING: Don't forget to make the following call in the derived Application class
        // Wait for GPU rendering is completed to release resources
        // m_context_ptr->WaitForGpu(RenderContext::WaitFor::RenderComplete)
        META_FUNCTION_TASK();
    }

    App& operator=(const App&) = delete;
    App& operator=(App&&) = delete;

    // Platform::App interface
    void InitContext(const Platform::AppEnvironment& env, const FrameSize& frame_size) override
    {
        META_FUNCTION_TASK();
        const Ptrs<Device>& devices = System::Get().UpdateGpuDevices();
        META_CHECK_ARG_NOT_EMPTY(devices);

        Ptr<Device> device_ptr = m_settings.default_device_index < 0
                      ? System::Get().GetSoftwareGpuDevice()
                      : (static_cast<size_t>(m_settings.default_device_index) < devices.size()
                           ? devices[m_settings.default_device_index]
                           : devices.front());
        META_CHECK_ARG_NOT_NULL(device_ptr);
        
        // Create render context of the current window size
        m_initial_context_settings.frame_size = frame_size;
        m_context_ptr = RenderContext::Create(env, *device_ptr, GetParallelExecutor(), m_initial_context_settings);
        m_context_ptr->SetName("App Render Context");
        m_context_ptr->Connect(*this);

        AddInputControllers({ std::make_shared<AppContextController>(*m_context_ptr) });
        
        SetFullScreen(m_initial_context_settings.is_full_screen);
    }

    void Init() override
    {
        META_FUNCTION_TASK();

        if (!m_settings.animations_enabled)
        {
            m_settings.animations_enabled = true;
            SetAnimationsEnabled(false);
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

        // Create frame resources
        for (uint32_t frame_index = 0; frame_index < context_settings.frame_buffers_count; ++frame_index)
        {
            FrameT frame(frame_index);

            // Create color texture for frame buffer
            frame.screen_texture_ptr = Texture::CreateFrameBuffer(*m_context_ptr, frame.index);
            frame.screen_texture_ptr->SetName(IndexedName("Frame Buffer", frame.index));

            // Configure render pass: color, depth, stencil attachments and shader access
            frame.screen_pass_ptr = RenderPass::Create(*m_context_ptr, {
                {
                    RenderPass::ColorAttachment(
                        {
                            frame.screen_texture_ptr, 0, 0, 0,
                            context_settings.clear_color.has_value()
                                ? RenderPass::Attachment::LoadAction::Clear
                                : RenderPass::Attachment::LoadAction::DontCare,
                            RenderPass::Attachment::StoreAction::Store,
                        },
                        context_settings.clear_color
                            ? *context_settings.clear_color
                            : Color4f()
                    )
                },
                RenderPass::DepthAttachment(
                    {
                        m_depth_texture_ptr, 0, 0, 0,
                        context_settings.clear_depth_stencil.has_value()
                            ? RenderPass::Attachment::LoadAction::Clear
                            : RenderPass::Attachment::LoadAction::DontCare,
                        RenderPass::Attachment::StoreAction::DontCare,
                    },
                    context_settings.clear_depth_stencil
                        ? context_settings.clear_depth_stencil->first
                        : 1.F
                ),
                RenderPass::StencilAttachment(),
                m_settings.screen_pass_access,
                true // final render pass
            });

            m_frames.emplace_back(std::move(frame));
        }

        Platform::App::Init();
    }

    void StartResizing() override
    {
        META_FUNCTION_TASK();
        Platform::App::StartResizing();
        m_restore_animations_enabled = m_settings.animations_enabled;
        SetAnimationsEnabled(false);
    }

    void EndResizing() override
    {
        META_FUNCTION_TASK();
        SetAnimationsEnabled(m_restore_animations_enabled);
        Platform::App::EndResizing();
    }

    bool Resize(const FrameSize& frame_size, bool is_minimized) override
    {
        META_FUNCTION_TASK();
        if (!AppBase::Resize(frame_size, is_minimized))
            return false;

        m_initial_context_settings.frame_size = frame_size;

        // Update viewports and scissor rects state
        m_view_state_ptr->SetViewports({ GetFrameViewport(frame_size) });
        m_view_state_ptr->SetScissorRects({ GetFrameScissorRect(frame_size) });

        // Save frame and depth textures restore information and delete obsolete resources
        std::vector<ResourceRestoreInfo> frame_restore_infos;
        frame_restore_infos.reserve(m_frames.size());
        for (FrameT& frame : m_frames)
        {
            frame_restore_infos.emplace_back(frame.screen_texture_ptr);
            frame.ReleaseScreenPassAttachmentTextures();
        }
        const ResourceRestoreInfo depth_restore_info(m_depth_texture_ptr);
        m_depth_texture_ptr.reset();

        // Resize render context
        META_CHECK_ARG_NOT_NULL(m_context_ptr);
        m_context_ptr->Resize(frame_size);

        // Restore depth texture with new size
        if (!depth_restore_info.descriptor_by_usage.empty())
        {
            m_depth_texture_ptr = Texture::CreateDepthStencilBuffer(*m_context_ptr, depth_restore_info.descriptor_by_usage);
            m_depth_texture_ptr->SetName(depth_restore_info.name);
        }

        // Restore frame buffers with new size and update textures in render pass settings
        for (FrameT& frame : m_frames)
        {
            ResourceRestoreInfo& frame_restore_info = frame_restore_infos[frame.index];
            RenderPass::Settings pass_settings      = frame.screen_pass_ptr->GetSettings();

            frame.screen_texture_ptr = Texture::CreateFrameBuffer(*m_context_ptr, frame.index, frame_restore_info.descriptor_by_usage);
            frame.screen_texture_ptr->SetName(frame_restore_info.name);

            pass_settings.color_attachments[0].texture_ptr = frame.screen_texture_ptr;
            pass_settings.depth_attachment.texture_ptr     = m_depth_texture_ptr;

            frame.screen_pass_ptr->Update(pass_settings);
        }

        return true;
    }
    
    bool Update() override
    {
        META_FUNCTION_TASK();
        if (IsMinimized())
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

    bool Render() override
    {
        META_FUNCTION_TASK();
        if (IsMinimized())
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
    
    bool SetFullScreen(bool is_full_screen) override
    {
        META_FUNCTION_TASK();
        if (m_context_ptr)
            m_context_ptr->SetFullScreen(is_full_screen);
        
        return Platform::App::SetFullScreen(is_full_screen);
    }

    // Graphics::IApp interface
    const Graphics::IApp::Settings& GetGraphicsAppSettings() const noexcept override { return m_settings; }

    bool SetAnimationsEnabled(bool animations_enabled) override
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

    void SetShowHudInWindowTitle(bool show_hud_in_window_title) noexcept
    {
        if (m_settings.show_hud_in_window_title == show_hud_in_window_title)
            return;

        m_settings.show_hud_in_window_title = show_hud_in_window_title;
        UpdateWindowTitle();
    }

protected:
    struct ResourceRestoreInfo
    {
        Resource::DescriptorByUsage descriptor_by_usage;
        std::string                 name;

        ResourceRestoreInfo() = default;
        ResourceRestoreInfo(const ResourceRestoreInfo&) = default;
        explicit ResourceRestoreInfo(const Ptr<Resource>& resource_ptr)
            : descriptor_by_usage(resource_ptr ? resource_ptr->GetDescriptorByUsage() : Resource::DescriptorByUsage())
            , name(resource_ptr ? resource_ptr->GetName() : std::string())
        { }
    };

    void UpdateWindowTitle()
    {
        if (!m_settings.show_hud_in_window_title)
        {
            SetWindowTitle(GetPlatformAppSettings().name);
            return;
        }

        META_CHECK_ARG_NOT_NULL(m_context_ptr);
        if (!m_context_ptr)
            return;

        const RenderContext::Settings& context_settings      = m_context_ptr->GetSettings();
        const FpsCounter&              fps_counter           = m_context_ptr->GetFpsCounter();
        const uint32_t                 average_fps           = fps_counter.GetFramesPerSecond();
        const FpsCounter::FrameTiming  average_frame_timing  = fps_counter.GetAverageFrameTiming();
        const std::string title = fmt::format("{:s}        {:d} FPS, {:.2f} ms, {:.2f}% CPU |  {:d} x {:d}  |  {:d} FB  |  VSync {:s}  |  {:s}  |  F1 - help",
                                              GetPlatformAppSettings().name,
                                              average_fps, average_frame_timing.GetTotalTimeMSec(), average_frame_timing.GetCpuTimePercent(),
                                              context_settings.frame_size.width, context_settings.frame_size.height,
                                              context_settings.frame_buffers_count, (context_settings.vsync_enabled ? "ON" : "OFF"),
                                              m_context_ptr->GetDevice().GetAdapterName());

        SetWindowTitle(title);
    }

    void CompleteInitialization()
    {
        m_context_ptr->CompleteInitialization();
    }

    // Platform::AppBase interface

    Platform::AppView GetView() const override
    {
        META_FUNCTION_TASK();
        return m_context_ptr->GetAppView();
    }

    // IContextCallback implementation

    void OnContextReleased(Context&) override
    {
        META_FUNCTION_TASK();
        m_restore_animations_enabled = m_settings.animations_enabled;
        SetAnimationsEnabled(false);

        m_frames.clear();
        m_depth_texture_ptr.reset();
        m_view_state_ptr.reset();

        Deinitialize();
    }

    void OnContextCompletingInitialization(Context&) override { }

    void OnContextInitialized(Context&) override
    {
        META_FUNCTION_TASK();
        Init();
        SetAnimationsEnabled(m_restore_animations_enabled);
    }

    inline FrameT& GetCurrentFrame()
    {
        META_FUNCTION_TASK();
        const uint32_t frame_index = m_context_ptr->GetFrameBufferIndex();
        META_CHECK_ARG_LESS(frame_index, m_frames.size());
        return m_frames[frame_index];
    }

    const RenderContext::Settings& GetInitialContextSettings() const { return m_initial_context_settings; }
    bool IsRenderContextInitialized()                                { return !!m_context_ptr; }
    const Ptr<RenderContext>& GetRenderContextPtr() const noexcept   { return m_context_ptr; }
    RenderContext& GetRenderContext() const noexcept                 { return *m_context_ptr; }
    const Ptr<ViewState>& GetViewStatePtr() const noexcept           { return m_view_state_ptr; }
    ViewState& GetViewState() noexcept                               { return *m_view_state_ptr; }

    FrameSize GetFrameSizeInDots() const noexcept { return m_context_ptr->GetSettings().frame_size / m_context_ptr->GetContentScalingFactor(); }

    static std::string IndexedName(const std::string& base_name, uint32_t index)
    {
        META_FUNCTION_TASK();
        return fmt::format("{} {}", base_name, index);
    }

    ImageLoader&          GetImageLoader() noexcept             { return m_image_loader; }
    Data::AnimationsPool& GetAnimations() noexcept              { return m_animations; }
    std::vector<FrameT>&  GetFrames() noexcept                  { return m_frames; }
    const Ptr<Texture>&   GetDepthTexturePtr() const noexcept   { return m_depth_texture_ptr; }

private:
    Graphics::IApp::Settings m_settings;
    RenderContext::Settings  m_initial_context_settings;
    Timer                    m_title_update_timer;
    bool                     m_restore_animations_enabled = true;
    ImageLoader              m_image_loader;
    Data::AnimationsPool     m_animations;
    Ptr<RenderContext>       m_context_ptr;
    Ptr<Texture>             m_depth_texture_ptr;
    Ptr<ViewState>           m_view_state_ptr;
    std::vector<FrameT>      m_frames;

    static constexpr double  g_title_update_interval_sec = 1.0;
};

} // namespace Methane::Graphics
