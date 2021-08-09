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
    ClearDepth  = 1U << 1U,
    ClearColor  = 1U << 2U,
    Animations  = 1U << 3U,
    Fullscreen  = 1U << 4U,
    VSync       = 1U << 5U
};


static constexpr AppOptions g_default_app_options_color_only = []()
{
    using namespace magic_enum::bitwise_operators;
    return AppOptions::ClearColor
#ifdef __APPLE__
           | AppOptions::VSync
#endif
    ;
}();

static constexpr AppOptions g_default_app_options_color_with_depth = []()
{
    using namespace magic_enum::bitwise_operators;
    return g_default_app_options_color_only |
           AppOptions::DepthBuffer |
           AppOptions::ClearDepth;
}();

static constexpr AppOptions g_default_app_options_color_with_depth_and_anim = []()
{
    using namespace magic_enum::bitwise_operators;
    return g_default_app_options_color_with_depth |
           AppOptions::Animations;
}();

static constexpr AppOptions g_default_app_options_color_only_and_anim = []()
{
    using namespace magic_enum::bitwise_operators;
    return g_default_app_options_color_only |
           AppOptions::Animations;
}();

static constexpr Graphics::RenderPass::Access g_default_screen_pass_access = []()
{
    using namespace magic_enum::bitwise_operators;
    return Graphics::RenderPass::Access::ShaderResources |
           Graphics::RenderPass::Access::Samplers;
}();

static constexpr Graphics::Context::Options g_default_context_options = Graphics::Context::Options::None;
static constexpr Graphics::DepthStencil     g_default_clear_depth_stencil(1.F, Graphics::Stencil(0));
static const     Graphics::Color4F          g_default_clear_color(0.0F, 0.2F, 0.4F, 1.0F);

[[nodiscard]] inline Graphics::AppSettings GetGraphicsAppSettings(const std::string& app_name, AppOptions app_options)
{
    using namespace magic_enum::bitwise_operators;
    using DepthStencilOpt = std::optional<Graphics::DepthStencil>;
    using ColorOpt        = std::optional<Graphics::Color4F>;

    const bool depth_enabled       = magic_enum::flags::enum_contains(app_options & AppOptions::DepthBuffer);
    const bool clear_depth_enabled = magic_enum::flags::enum_contains(app_options & AppOptions::ClearDepth);
    const bool clear_color_enabled = magic_enum::flags::enum_contains(app_options & AppOptions::ClearColor);
    const bool animations_enabled  = magic_enum::flags::enum_contains(app_options & AppOptions::Animations);
    const bool fullscreen_enabled  = magic_enum::flags::enum_contains(app_options & AppOptions::Fullscreen);
    const bool vsync_enabled       = magic_enum::flags::enum_contains(app_options & AppOptions::VSync);

    return Graphics::AppSettings
    {                                                           // =========================
        Platform::App::Settings {                               // platform_app:
            app_name,                                           //   - name
            { 0.8, 0.8 },                                       //   - width, height
        },                                                      // =========================
        Graphics::IApp::Settings {                              // graphics_app:
            g_default_screen_pass_access,                       //   - screen_pass_access
            animations_enabled,                                 //   - animations_enabled
            true,                                               //   - show_hud_in_window_title
            0                                                   //   - default_device_index
        },                                                      // =========================
        Graphics::RenderContext::Settings {                     // render_context:
            Graphics::FrameSize(),                              //   - frame_size
            Graphics::PixelFormat::BGRA8Unorm,                  //   - color_format
            depth_enabled                                       //   - depth_stencil_format
                ? Graphics::PixelFormat::Depth32Float           //     ...
                : Graphics::PixelFormat::Unknown,               //     ...
            clear_color_enabled                                 //   - clear_color
                ? ColorOpt(g_default_clear_color)               //     ...
                : ColorOpt(),                                   //     ...
            depth_enabled && clear_depth_enabled                //   - clear_depth_stencil
                ? DepthStencilOpt(g_default_clear_depth_stencil)//     ...
                : DepthStencilOpt(),                            //     ...
            3U,                                                 //   - frame_buffers_count
            vsync_enabled,                                      //   - vsync_enabled
            fullscreen_enabled,                                 //   - is_full_screen
            g_default_context_options,                          //   - options_mask
            1000U,                                              //   - unsync_max_fps (MacOS only)
        }                                                       // =========================
    };
}

} // namespace Methane::Graphics
