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
#include <Methane/Timer.hpp>
#include <Methane/Data/AnimationsPool.h>
#include <Methane/Platform/App.h>
#include <Methane/Graphics/Types.h>
#include <Methane/Graphics/Device.h>
#include <Methane/Graphics/RenderContext.h>
#include <Methane/Graphics/Texture.h>
#include <Methane/Graphics/RenderCommandList.h>
#include <Methane/Graphics/FpsCounter.h>
#include <Methane/Graphics/ImageLoader.h>
#include <Methane/Graphics/LogoBadge.h>
#include <Methane/Instrumentation.h>

#include <vector>
#include <sstream>
#include <memory>
#include <thread>
#include <cassert>

namespace Methane::Graphics
{

struct AppFrame
{
    const uint32_t  index = 0;
    Ptr<Texture>    sp_screen_texture;
    Ptr<RenderPass> sp_screen_pass;

    AppFrame(uint32_t frame_index) : index(frame_index) { ITT_FUNCTION_TASK(); }
};

template<typename FrameT>
class App
    : public Graphics::IApp
    , public Platform::App
    , public RenderContext::Callback
{
    static_assert(std::is_base_of<AppFrame, FrameT>::value, "Application Frame type must be derived from AppFrame.");

public:

    struct AllSettings
    {
        Platform::App::Settings platform_app;
        IApp::Settings          graphics_app;
        RenderContext::Settings render_context;
    };

    explicit App(const AllSettings& settings, const std::string& help_description = "Methane Graphics Application")
        : Platform::App(settings.platform_app)
        , m_image_loader(Data::TextureProvider::Get())
        , m_settings(settings.graphics_app)
        , m_initial_context_settings(settings.render_context)
    {
        ITT_FUNCTION_TASK();
        add_option("-i,--hud", m_settings.show_hud_in_window_title, "HUD information in window title", true);
        add_option("-a,--animations", m_settings.animations_enabled, "Enable animations", true);
        add_option("-d,--device", m_settings.default_device_index, "Render at adapter index, use -1 for software adapter", true);
        add_option("-v,--vsync", m_initial_context_settings.vsync_enabled, "Vertical synchronization", true);
        add_option("-b,--frame-buffers", m_initial_context_settings.frame_buffers_count, "Frame buffers count in swap-chain", true);

        InputState().AddControllers({ std::make_shared<AppController>(*this, help_description) });
    }

    ~App() override
    {
        // WARNING: Don't forget to make the following call in the derived Application class
        // Wait for GPU rendering is completed to release resources
        // m_sp_context->WaitForGpu(RenderContext::WaitFor::RenderComplete);
        ITT_FUNCTION_TASK();
        m_sp_context->RemoveCallback(*this);
    }

    // Platform::App interface
    void InitContext(const Platform::AppEnvironment& env, const FrameSize& frame_size) override
    {
        ITT_FUNCTION_TASK();
        const Ptrs<Device>& devices = System::Get().UpdateGpuDevices();
        assert(!devices.empty());

        Ptr<Device> sp_device = m_settings.default_device_index < 0
                      ? System::Get().GetSoftwareGpuDevice()
                      : (static_cast<size_t>(m_settings.default_device_index) < devices.size()
                           ? devices[m_settings.default_device_index]
                           : devices.front());
        assert(sp_device);
        
        // Create render context of the current window size
        m_initial_context_settings.frame_size = frame_size;
        m_sp_context = RenderContext::Create(env, *sp_device, m_initial_context_settings);
        m_sp_context->SetName("App Render Context");
        m_sp_context->AddCallback(*this);

        InputState().AddControllers({ std::make_shared<AppContextController>(*m_sp_context) });
        
        SetFullScreen(m_initial_context_settings.is_full_screen);
    }

    void Init() override
    {
        ITT_FUNCTION_TASK();
        assert(m_sp_context);
        const RenderContext::Settings& context_settings = m_sp_context->GetSettings();

        // Create depth texture for FB rendering
        if (context_settings.depth_stencil_format != PixelFormat::Unknown)
        {
            m_sp_depth_texture = Texture::CreateDepthStencilBuffer(*m_sp_context);
            m_sp_depth_texture->SetName("Depth Texture");
        }

        // Create frame resources
        for (uint32_t frame_index = 0; frame_index < context_settings.frame_buffers_count; ++frame_index)
        {
            FrameT frame(frame_index);

            // Create color texture for frame buffer
            frame.sp_screen_texture = Texture::CreateFrameBuffer(*m_sp_context, frame.index);
            frame.sp_screen_texture->SetName(IndexedName("Frame Buffer", frame.index));

            // Configure render pass: color, depth, stencil attachments and shader access
            frame.sp_screen_pass = RenderPass::Create(*m_sp_context, {
                {
                    RenderPass::ColorAttachment(
                        {
                            frame.sp_screen_texture, 0, 0, 0,
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
                        m_sp_depth_texture, 0, 0, 0,
                        context_settings.clear_depth_stencil.has_value()
                            ? RenderPass::Attachment::LoadAction::Clear
                            : RenderPass::Attachment::LoadAction::DontCare,
                        RenderPass::Attachment::StoreAction::DontCare,
                    },
                    context_settings.clear_depth_stencil
                        ? context_settings.clear_depth_stencil->first
                        : 1.f
                ),
                RenderPass::StencilAttachment(),
                m_settings.screen_pass_access
                });

            m_frames.emplace_back(std::move(frame));
        }
        
        // Create Methane logo badge
        if (m_settings.show_logo_badge)
            m_sp_logo_badge = std::make_shared<LogoBadge>(*m_sp_context);

        Platform::App::Init();
    }

    bool Resize(const FrameSize& frame_size, bool is_minimized) override
    {
        ITT_FUNCTION_TASK();

        if (!AppBase::Resize(frame_size, is_minimized))
            return false;

        m_initial_context_settings.frame_size = frame_size;

        // Save frame and depth textures restore information and delete obsolete resources
        std::vector<ResourceRestoreInfo> frame_restore_infos;
        for (FrameT& frame : m_frames)
        {
            frame_restore_infos.emplace_back(frame.sp_screen_texture);
            frame.sp_screen_texture.reset();
        }
        const ResourceRestoreInfo depth_restore_info(m_sp_depth_texture);
        m_sp_depth_texture.reset();

        // Resize render context
        assert(m_sp_context);
        m_sp_context->Resize(frame_size);

        // Restore depth texture with new size
        if (!depth_restore_info.descriptor_by_usage.empty())
        {
            m_sp_depth_texture = Texture::CreateDepthStencilBuffer(*m_sp_context, depth_restore_info.descriptor_by_usage);
            m_sp_depth_texture->SetName(depth_restore_info.name);
        }

        // Restore frame buffers with new size and update textures in render pass settings
        for (FrameT& frame : m_frames)
        {
            ResourceRestoreInfo& frame_restore_info = frame_restore_infos[frame.index];
            RenderPass::Settings pass_settings      = frame.sp_screen_pass->GetSettings();

            frame.sp_screen_texture = Texture::CreateFrameBuffer(*m_sp_context, frame.index, frame_restore_info.descriptor_by_usage);
            frame.sp_screen_texture->SetName(frame_restore_info.name);

            pass_settings.color_attachments[0].wp_texture = frame.sp_screen_texture;
            pass_settings.depth_attachment.wp_texture     = m_sp_depth_texture;

            frame.sp_screen_pass->Update(pass_settings);
        }
        
        if (m_sp_logo_badge)
            m_sp_logo_badge->Resize(frame_size);

        return true;
    }
    
    bool Update() override
    {
        ITT_FUNCTION_TASK();
        if (IsMinimized())
            return false;
        
        System::Get().CheckForChanges();

        m_animations.Update();
        return true;
    }

    bool Render() override
    {
        ITT_FUNCTION_TASK();
        
        if (IsMinimized())
        {
            // No need to render frames while window is minimized.
            // Sleep thread for a while to not heat CPU by running the message loop
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return false;
        }

        if (!m_sp_context)
        {
            throw std::runtime_error("RenderContext is not initialized before rendering.");
        }

        // Update HUD info in window title
        if (m_settings.show_hud_in_window_title &&
            m_title_update_timer.GetElapsedSecondsD() >= g_title_update_interval_sec)
        {
            UpdateWindowTitle();
            m_title_update_timer.Reset();
        }

        return true;
    }
    
    void RenderOverlay(RenderCommandList& cmd_list)
    {
        ITT_FUNCTION_TASK();
        if (m_sp_logo_badge)
            m_sp_logo_badge->Draw(cmd_list);
    }
    
    bool SetFullScreen(bool is_full_screen) override
    {
        ITT_FUNCTION_TASK();
        if (m_sp_context)
            m_sp_context->SetFullScreen(is_full_screen);
        
        return Platform::App::SetFullScreen(is_full_screen);
    }

    // Context::Callback interface
    void OnContextReleased() override
    {
        ITT_FUNCTION_TASK();
        m_frames.clear();
        m_sp_depth_texture.reset();
        m_sp_logo_badge.reset();
        Deinitialize();
    }

    // Context::Callback interface
    void OnContextInitialized() override
    {
        ITT_FUNCTION_TASK();
        Init();
    }

    // Graphics::IApp interface
    const IApp::Settings& GetGraphicsAppSettings() const noexcept override        { return m_settings; }

    bool SetAnimationsEnabled(bool animations_enabled) override
    {
        if (m_settings.animations_enabled == animations_enabled)
            return false;

        m_settings.animations_enabled = animations_enabled;

        // Pause animations or resume from the paused state
        if (m_settings.animations_enabled)
            m_animations.Resume();
        else
            m_animations.Pause();

        // Disable all camera controllers while animations are paused, since they can not function without animations
        Refs<AppCameraController> camera_controllers = InputState().template GetControllersOfType<AppCameraController>();
        for(const Ref<AppCameraController> camera_controller : camera_controllers)
        {
            camera_controller.get().SetEnabled(animations_enabled);
        }

        return true;
    }

    bool SetShowHudInWindowTitle(bool show_hud_in_window_title) override
    {
        if (m_settings.show_hud_in_window_title == show_hud_in_window_title)
            return false;

        m_settings.show_hud_in_window_title = show_hud_in_window_title;
        UpdateWindowTitle();

        return true;
    }

protected:
    struct ResourceRestoreInfo
    {
        Resource::DescriptorByUsage descriptor_by_usage;
        std::string                 name;

        ResourceRestoreInfo() = default;
        ResourceRestoreInfo(const ResourceRestoreInfo&) = default;
        explicit ResourceRestoreInfo(const Ptr<Resource>& sp_resource)
            : descriptor_by_usage(sp_resource ? sp_resource->GetDescriptorByUsage() : Resource::DescriptorByUsage())
            , name(sp_resource ? sp_resource->GetName() : std::string())
        { }
    };

    void UpdateWindowTitle()
    {
        if (!m_settings.show_hud_in_window_title)
        {
            SetWindowTitle(GetPlatformAppSettings().name);
            return;
        }

        assert(m_sp_context);
        if (!m_sp_context)
            return;

        const RenderContext::Settings& context_settings      = m_sp_context->GetSettings();
        const FpsCounter&              fps_counter           = m_sp_context->GetFpsCounter();
        const uint32_t                 average_fps           = fps_counter.GetFramesPerSecond();
        const FpsCounter::FrameTiming  average_frame_timing  = fps_counter.GetAverageFrameTiming();

        std::stringstream title_ss;
        title_ss.precision(2);
        title_ss << GetPlatformAppSettings().name
                 << "        "  << average_fps
                 << " FPS, "    << std::fixed << average_frame_timing.GetTotalTimeMSec()
                 << " ms, "     << std::fixed << average_frame_timing.GetCpuTimePercent() << "% cpu"
                 << "  |  "        << context_settings.frame_size.width << " x " << context_settings.frame_size.height
                 << "  |  "        << std::to_string(context_settings.frame_buffers_count) << " FB"
                 << "  |  VSync " << (context_settings.vsync_enabled ? "ON" : "OFF")
                 << "  |  "   << m_sp_context->GetDevice().GetAdapterName()
                 << "  |  F1 - help";

        SetWindowTitle(title_ss.str());
    }

    // AppBase interface

    Platform::AppView GetView() const override
    {
        ITT_FUNCTION_TASK();
        return m_sp_context->GetAppView();
    }

    inline FrameT& GetCurrentFrame()
    {
        ITT_FUNCTION_TASK();
        const uint32_t frame_index = m_sp_context->GetFrameBufferIndex();
        assert(frame_index < m_frames.size());
        return m_frames[frame_index];
    }

    const RenderContext::Settings& GetInitialContextSettings() const { return m_initial_context_settings; }

    static std::string IndexedName(const std::string& base_name, uint32_t index)
    {
        ITT_FUNCTION_TASK();
        std::stringstream ss;
        ss << base_name << " " << std::to_string(index);
        return ss.str();
    }

    ImageLoader             m_image_loader;
    Data::AnimationsPool    m_animations;

    Ptr<RenderContext>      m_sp_context;
    Ptr<Texture>            m_sp_depth_texture;
    Ptr<LogoBadge>          m_sp_logo_badge;
    std::vector<FrameT>     m_frames;

private:
    IApp::Settings          m_settings;
    RenderContext::Settings m_initial_context_settings;
    Timer                   m_title_update_timer;

    static constexpr double g_title_update_interval_sec = 1.0;
};

} // namespace Methane::Graphics
