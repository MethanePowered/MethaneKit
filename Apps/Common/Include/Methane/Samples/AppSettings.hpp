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

#include <Methane/Graphics/App.h>
#include <Methane/UserInterface/App.h>
#include <Methane/Data/AppIconsProvider.h>

#include <magic_enum.hpp>

namespace Methane::Tutorials
{

enum class AppOptions : uint32_t
{
    None        = 0U,
    DepthBuffer = 1U << 0U,
    ClearDepth  = 1U << 1U,
    ClearColor  = 1U << 2U,
    Animations  = 1U << 3U,
    Fullscreen  = 1U << 4U,
    VSync       = 1U << 5U,
    HudUi       = 1U << 6U
};

static constexpr AppOptions g_default_app_options_color_only = []()
{
    using namespace magic_enum::bitwise_operators;
    return AppOptions::ClearColor
#ifdef __APPLE__
         | AppOptions::VSync
#ifndef APPLE_MACOS // iOS
         | AppOptions::Fullscreen
         | AppOptions::HudUi
#endif
#endif
    ;
}();

static constexpr AppOptions g_default_app_options_color_with_depth = []()
{
    using namespace magic_enum::bitwise_operators;
    return g_default_app_options_color_only
         | AppOptions::DepthBuffer
         | AppOptions::ClearDepth;
}();

static constexpr AppOptions g_default_app_options_color_with_depth_and_anim = []()
{
    using namespace magic_enum::bitwise_operators;
    return g_default_app_options_color_with_depth
         | AppOptions::Animations;
}();

static constexpr AppOptions g_default_app_options_color_only_and_anim = []()
{
    using namespace magic_enum::bitwise_operators;
    return g_default_app_options_color_only
         | AppOptions::Animations;
}();

static constexpr Graphics::RenderPass::Access g_default_screen_pass_access = []()
{
    using namespace magic_enum::bitwise_operators;
    return Graphics::RenderPass::Access::ShaderResources
         | Graphics::RenderPass::Access::Samplers;
}();

static constexpr Graphics::IContext::Options g_default_context_options = Graphics::IContext::Options::None;
static constexpr Graphics::DepthStencil     g_default_clear_depth_stencil(1.F, Graphics::Stencil(0));
static const     Graphics::Color4F          g_default_clear_color(0.0F, 0.2F, 0.4F, 1.0F);

[[nodiscard]] inline Graphics::AppSettings GetGraphicsTutorialAppSettings(const std::string& app_name, AppOptions app_options)
{
    using namespace magic_enum::bitwise_operators;
    using DepthStencilOpt = std::optional<Graphics::DepthStencil>;
    using ColorOpt        = std::optional<Graphics::Color4F>;

    const auto depth_enabled       = static_cast<bool>(app_options & AppOptions::DepthBuffer);
    const auto clear_depth_enabled = static_cast<bool>(app_options & AppOptions::ClearDepth);
    const auto clear_color_enabled = static_cast<bool>(app_options & AppOptions::ClearColor);
    const auto animations_enabled  = static_cast<bool>(app_options & AppOptions::Animations);
    const auto fullscreen_enabled  = static_cast<bool>(app_options & AppOptions::Fullscreen);
    const auto vsync_enabled       = static_cast<bool>(app_options & AppOptions::VSync);
    const auto hud_ui_enabled      = static_cast<bool>(app_options & AppOptions::HudUi);

    return Graphics::AppSettings
    {                                                           // =========================
        Platform::IApp::Settings {                               // platform_app:
            app_name,                                           //   - name
            { 0.8, 0.8 },                                       //   - size
            { 640U, 480U },                                     //   - min_size
            fullscreen_enabled,                                 //   - is_full_screen
            &Data::IconProvider::Get(),                         //   - icon_resources_ptr
        },                                                      // =========================
        Graphics::IApp::Settings {                              // graphics_app:
            g_default_screen_pass_access,                       //   - screen_pass_access
            animations_enabled,                                 //   - animations_enabled
            !hud_ui_enabled,                                    //   - show_hud_in_window_title
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

[[nodiscard]] inline UserInterface::IApp::Settings GetUserInterfaceTutorialAppSettings(AppOptions app_options)
{
    using namespace magic_enum::bitwise_operators;
    const auto hud_ui_enabled      = static_cast<bool>(app_options & AppOptions::HudUi);

    return UserInterface::IApp::Settings
    {
        hud_ui_enabled ? UserInterface::IApp::HeadsUpDisplayMode::UserInterface : UserInterface::IApp::HeadsUpDisplayMode::WindowTitle,
        true // badge_visible
    };
}

} // namespace Methane::Graphics
