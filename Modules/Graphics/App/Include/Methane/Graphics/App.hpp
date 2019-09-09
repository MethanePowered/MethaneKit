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

FILE: Methane/Graphics/App.hpp
Base template class of the Methane graphics application with multiple frame buffers.
Base frame class provides frame buffer management with resize handling.

******************************************************************************/

#pragma once

#include "AppDataProvider.hpp"
#include "AppContextController.h"

#include <Methane/Data/Timer.h>
#include <Methane/Data/AnimationsPool.h>
#include <Methane/Platform/App.h>
#include <Methane/Platform/AppController.h>
#include <Methane/Graphics/Types.h>
#include <Methane/Graphics/Device.h>
#include <Methane/Graphics/Context.h>
#include <Methane/Graphics/Texture.h>
#include <Methane/Graphics/RenderPass.h>
#include <Methane/Graphics/FpsCounter.h>
#include <Methane/Graphics/ImageLoader.h>
#include <Methane/Instrumentation.h>

#include <vector>
#include <sstream>
#include <memory>
#include <cassert>

namespace Methane::Graphics
{

struct AppFrame
{
    const uint32_t  index = 0;
    Texture::Ptr    sp_screen_texture;
    RenderPass::Ptr sp_screen_pass;

    AppFrame(uint32_t frame_index) : index(frame_index) { ITT_FUNCTION_TASK(); }
};

template<typename FrameT>
class App
    : public Platform::App
    , public Context::Callback
{
    static_assert(std::is_base_of<AppFrame, FrameT>::value, "Application Frame type must be derived from AppFrame.");

public:
    struct Settings
    {
        Platform::App::Settings app;
        Context::Settings       context;
        bool                    show_hud_in_window_title;
    };

    App(const Settings& settings, RenderPass::Access::Mask screen_pass_access,
        const std::string& help_description = "Methane Graphics Application")
        : Platform::App(settings.app)
        , m_image_loader(AppDataProvider::Get())
        , m_initial_context_settings(settings.context)
        , m_screen_pass_access(screen_pass_access)
        , m_show_hud_in_window_title(settings.show_hud_in_window_title)
    {
        ITT_FUNCTION_TASK();
        m_cmd_options.add_options()
            ("d,hud", "Show/hide HUD in window title", cxxopts::value<int>())
            ("v,vsync", "Enable/disable vertical synchronization", cxxopts::value<int>())
            ("i,device", "Render at adapter index, use -1 for software adapter", cxxopts::value<int>())
            ("f,framebuffers", "Frame buffers count in swap-chain", cxxopts::value<uint32_t>());

        m_input_state.AddControllers({ std::make_shared<Platform::AppController>(*this, help_description) });
    }

    ~App() override
    {
        // WARNING: Don't forget to make the following call in the derived Application class
        // Wait for GPU rendering is completed to release resources
        // m_sp_context->WaitForGpu(Context::WaitFor::RenderComplete);
        ITT_FUNCTION_TASK();
        m_sp_context->RemoveCallback(*this);
    }

    // Platform::App interface
    void InitContext(const Platform::AppEnvironment& env, const FrameSize& frame_size) override
    {
        ITT_FUNCTION_TASK();
        const Devices& devices = System::Get().UpdateGpuDevices();
        assert(!devices.empty());

        Device::Ptr sp_device = m_default_device_index < 0
                      ? System::Get().GetSoftwareGpuDevice()
                      : (static_cast<size_t>(m_default_device_index) < devices.size()
                           ? devices[m_default_device_index]
                           : devices.front());
        assert(sp_device);
        
        // Create render context of the current window size
        m_initial_context_settings.frame_size = frame_size;
        m_sp_context = Context::Create(env, AppDataProvider::Get(), *sp_device, m_initial_context_settings);
        m_sp_context->SetName("App Gfx Context");
        m_sp_context->AddCallback(*this);

        m_input_state.AddControllers({ std::make_shared<AppContextController>(*m_sp_context) });
        
        SetFullScreen(m_initial_context_settings.is_full_screen);
    }

    void Init() override
    {
        ITT_FUNCTION_TASK();
        assert(m_sp_context);
        const Context::Settings& context_settings = m_sp_context->GetSettings();

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
                            frame.sp_screen_texture,
                            0, 0, 0,
                            RenderPass::Attachment::LoadAction::Clear,
                            RenderPass::Attachment::StoreAction::Store,
                        },
                        context_settings.clear_color
                    )
                },
                RenderPass::DepthAttachment(
                    {
                        m_sp_depth_texture,
                        0, 0, 0,
                        RenderPass::Attachment::LoadAction::Clear,
                        RenderPass::Attachment::StoreAction::DontCare,
                    },
                    context_settings.clear_depth
                ),
                RenderPass::StencilAttachment(),
                m_screen_pass_access
                });

            m_frames.emplace_back(std::move(frame));
        }

        Platform::App::Init();
    }

    bool Resize(const FrameSize& frame_size, bool /*is_minimized*/) override
    {
        ITT_FUNCTION_TASK();
        struct ResourceInfo
        {
            Resource::DescriptorByUsage descriptor_by_usage;
            std::string name;
        };

        if (!m_initialized || m_initial_context_settings.frame_size == frame_size)
            return false;

        m_initial_context_settings.frame_size = frame_size;

        // Save color texture information and delete obsolete resources for each frame buffer
        std::vector<ResourceInfo> frame_restore_info(m_frames.size());
        for (FrameT& frame : m_frames)
        {
            assert(!!frame.sp_screen_texture);

            frame_restore_info[frame.index] = {
                frame.sp_screen_texture->GetDescriptorByUsage(),
                frame.sp_screen_texture->GetName()
            };

            frame.sp_screen_texture.reset();
        }

        // Save depth texture information and delete it
        const Resource::DescriptorByUsage depth_descriptor_by_usage = m_sp_depth_texture ? m_sp_depth_texture->GetDescriptorByUsage() : Resource::DescriptorByUsage();
        const std::string depth_resource_name = m_sp_depth_texture ? m_sp_depth_texture->GetName() : std::string();
        m_sp_depth_texture.reset();

        // Resize render context
        assert(m_sp_context);
        m_sp_context->Resize(frame_size);

        // Resize depth texture and update it in render pass
        if (!depth_descriptor_by_usage.empty())
        {
            m_sp_depth_texture = Texture::CreateDepthStencilBuffer(*m_sp_context, depth_descriptor_by_usage);
            m_sp_depth_texture->SetName(depth_resource_name);
        }

        // Resize frame buffers by creating new color textures and updating them in render pass
        for (FrameT& frame : m_frames)
        {
            ResourceInfo& frame_info = frame_restore_info[frame.index];
            RenderPass::Settings pass_settings = frame.sp_screen_pass->GetSettings();

            frame.sp_screen_texture = Texture::CreateFrameBuffer(*m_sp_context, frame.index, frame_info.descriptor_by_usage);
            frame.sp_screen_texture->SetName(frame_info.name);

            pass_settings.color_attachments[0].wp_texture = frame.sp_screen_texture;
            pass_settings.depth_attachment.wp_texture = m_sp_depth_texture;

            frame.sp_screen_pass->Update(pass_settings);
        }

        return true;
    }
    
    void Update() override
    {
        ITT_FUNCTION_TASK();
        System::Get().CheckForChanges();
        m_animations.Update();
    }

    void Render() override
    {
        ITT_FUNCTION_TASK();

        // Update HUD info in window title
        if (!m_show_hud_in_window_title ||
            m_title_update_timer.GetElapsedSecondsD() < g_title_update_interval_sec)
            return;

        if (!m_sp_context)
        {
            throw std::runtime_error("Context is not initialized before rendering.");
        }

        const Context::Settings& context_settings       = m_sp_context->GetSettings();
        const FpsCounter&        fps_counter            = m_sp_context->GetFpsCounter();
        const uint32_t           average_fps            = fps_counter.GetFramesPerSecond();
        const double             average_frame_time_ms  = fps_counter.GetAverageFrameTimeMilSec();

        std::stringstream title_ss;
        title_ss.precision(2);
        title_ss << m_settings.name << "        " 
                 << average_fps << " FPS (" << std::fixed << average_frame_time_ms << " ms)"
                 << ", " << context_settings.frame_size.width << " x " << context_settings.frame_size.height
                 << ", " << std::to_string(context_settings.frame_buffers_count) << " FBs"
                 << ", VSync: " << (context_settings.vsync_enabled ? "ON" : "OFF")
                 << ", GPU: "   << m_sp_context->GetDevice().GetAdapterName();

        SetWindowTitle(title_ss.str());
        m_title_update_timer.Reset();

        // Keep window full-screen mode in sync with the context
        SetFullScreen(context_settings.is_full_screen);
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
        m_initialized = false;
    }

    // Context::Callback interface
    void OnContextInitialized() override
    {
        ITT_FUNCTION_TASK();
        Init();
    }

    void SetShowHudInWindowTitle(bool show_hud_in_window_title) { m_show_hud_in_window_title = show_hud_in_window_title; }
    bool GetShowHudInWindowTitle() const                        { return m_show_hud_in_window_title; }

protected:

    // AppBase interface

    Platform::AppView GetView() const override
    {
        ITT_FUNCTION_TASK();
        return m_sp_context->GetAppView();
    }

    void ParseCommandLine(const cxxopts::ParseResult& cmd_parse_result) override
    {
        ITT_FUNCTION_TASK();
        Platform::App::ParseCommandLine(cmd_parse_result);

        if (cmd_parse_result.count("hud"))
        {
            m_show_hud_in_window_title = cmd_parse_result["hud"].as<int>() != 0;
        }
        if (cmd_parse_result.count("vsync"))
        {
            m_initial_context_settings.vsync_enabled = cmd_parse_result["vsync"].as<int>() != 0;
        }
        if (cmd_parse_result.count("device"))
        {
            m_default_device_index = cmd_parse_result["device"].as<int>();
        }
        if (cmd_parse_result.count("framebuffers"))
        {
            m_initial_context_settings.frame_buffers_count = cmd_parse_result["framebuffers"].as<uint32_t>();
        }
    }

    inline FrameT& GetCurrentFrame()
    {
        ITT_FUNCTION_TASK();
        const uint32_t frame_index = m_sp_context->GetFrameBufferIndex();
        assert(frame_index < m_frames.size());
        return m_frames[frame_index];
    }

    const Context::Settings& GetInitialContextSettings() const { return m_initial_context_settings; }

    static std::string IndexedName(const std::string& base_name, uint32_t index)
    {
        ITT_FUNCTION_TASK();
        std::stringstream ss;
        ss << base_name << " " << std::to_string(index);
        return ss.str();
    }

    Context::Ptr                    m_sp_context;
    ImageLoader                     m_image_loader;
    Texture::Ptr                    m_sp_depth_texture;
    std::vector<FrameT>             m_frames;
    Data::AnimationsPool            m_animations;

private:
    Context::Settings               m_initial_context_settings;
    int32_t                         m_default_device_index = 0;
    const RenderPass::Access::Mask  m_screen_pass_access;
    bool                            m_show_hud_in_window_title;
    Data::Timer                     m_title_update_timer;

    static constexpr double  g_title_update_interval_sec = 1;
};

} // namespace Methane::Graphics
