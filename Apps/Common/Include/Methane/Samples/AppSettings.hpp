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

enum class AppOptions : uint32_t
{
    None        = 0U,
    DepthBuffer = 1U << 0U,
    Animations  = 1U << 1U,
    Fullscreen  = 1U << 2U,
    Default     = static_cast<uint32_t>(DepthBuffer) | static_cast<uint32_t>(Animations)
};

[[nodiscard]] inline Graphics::AppSettings GetGraphicsAppSettings(
                                                const std::string& app_name,
                                                AppOptions app_options = AppOptions::Default,
                                                Graphics::Context::Options context_options = Graphics::Context::Options::Default,
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

    const bool depth_enabled      = magic_enum::flags::enum_contains(app_options & AppOptions::DepthBuffer);
    const bool animations_enabled = magic_enum::flags::enum_contains(app_options & AppOptions::Animations);
    const bool is_fullscreen      = magic_enum::flags::enum_contains(app_options & AppOptions::Fullscreen);

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
            is_apple,                                               //   - vsync_enabled
            is_fullscreen,                                          //   - is_full_screen
            context_options,                                        //   - options_mask
            1000U,                                                  //   - unsync_max_fps (MacOS only)
        }                                                           // =========================
    };
}

} // namespace Methane::Graphics
