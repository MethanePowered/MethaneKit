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

FILE: Methane/Samples/AppSettings.hpp
Common application settings for Methane samples and tutorials.

******************************************************************************/

#pragma once

#include <Methane/Graphics/App.hpp>

#include <magic_enum.hpp>

namespace Methane::Samples
{

[[nodiscard]] inline Graphics::AppSettings GetGraphicsAppSettings(
                                                const std::string& app_name,
                                                bool animations_enabled = true,
                                                bool depth_enabled = true,
                                                float clear_depth = 1.F,
                                                std::optional<Graphics::Color4F> clear_color = Graphics::Color4F(0.0F, 0.2F, 0.4F, 1.0F))
{
    using namespace magic_enum::bitwise_operators;
    using Stencil = Graphics::Stencil;
    using DepthStencilOpt = std::optional<Graphics::DepthStencil>;

#ifdef __APPLE__
    constexpr bool is_apple = true;
#else
    constexpr bool is_apple = false;
#endif

    return Graphics::AppSettings
    {                                                               // =========================
        Platform::App::Settings {                                   // platform_app:
            app_name,                                               //   - name
            0.8, 0.8,                                               //   - width, height
            false,                                                  //   - is_full_screen
        },                                                          // =========================
        Graphics::IApp::Settings {                                  // graphics_app:
            Graphics::RenderPass::Access::ShaderResources |         //   - screen_pass_access
            Graphics::RenderPass::Access::Samplers,                 //     ...
            animations_enabled,                                     //   - animations_enabled
            true,                                                   //   - show_hud_in_window_title
            0                                                       //   - default_device_index
        },                                                          // =========================
        Graphics::RenderContext::Settings {                         // render_context:
            Graphics::FrameSize(),                                  //   - frame_size
            Graphics::PixelFormat::BGRA8Unorm,                      //   - color_format
            depth_enabled                                           //   - depth_stencil_format
                ? Graphics::PixelFormat::Depth32Float               //     ...
                : Graphics::PixelFormat::Unknown,                   //     ...
            std::move(clear_color),                                 //   - clear_color
            depth_enabled                                           //   - clear_depth_stencil
                ? DepthStencilOpt({ clear_depth, Stencil(0) })      //     ...
                : DepthStencilOpt(),                                //     ...
            3U,                                                     //   - frame_buffers_count
            is_apple,                                             //   - vsync_enabled
            false,                                                  //   - is_full_screen
            false,                                                  //   - is_emulated_render_pass
            1000U,                                                  //   - unsync_max_fps (MacOS only)
        }                                                           // =========================
    };
}

} // namespace Methane::Graphics
