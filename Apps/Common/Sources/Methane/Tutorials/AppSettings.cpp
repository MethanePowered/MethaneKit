/******************************************************************************

Copyright 2020-2022 Evgeny Gorodetskiy

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

FILE: Methane/Tutorials/AppSettings.h
Common application settings for Methane samples and tutorials.

******************************************************************************/

#include <Methane/Tutorials/AppSettings.h>
#include <Methane/Data/AppIconsProvider.h>

#include <magic_enum.hpp>

namespace Methane::Tutorials
{

namespace rhi = Methane::Graphics::Rhi;

AppOptions AppOptions::GetDefaultWithColorOnly() noexcept
{
    AppOptions options;
    options.clear_color = true;
#ifdef __APPLE__
    options.vsync = true;
#ifndef APPLE_MACOS // iOS
    options.full_screen = true;
    options.hud_visible = true;
#endif
#endif
    return options;
}

AppOptions AppOptions::GetDefaultWithColorDepth() noexcept
{
    AppOptions options(GetDefaultWithColorOnly());
    options.depth_buffer = true;
    options.clear_depth = true;
    return options;
}

AppOptions AppOptions::GetDefaultWithColorDepthAndAnim() noexcept
{
    AppOptions options(GetDefaultWithColorDepth());
    options.animations = true;
    return options;
}

AppOptions AppOptions::GetDefaultWithColorOnlyAndAnim() noexcept
{
    AppOptions options(GetDefaultWithColorOnly());
    options.animations = true;
    return options;
}

Graphics::CombinedAppSettings GetGraphicsTutorialAppSettings(const std::string& app_name, AppOptions app_options)
{
    using DepthStencilOpt = std::optional<Graphics::DepthStencil>;
    using ColorOpt        = std::optional<Graphics::Color4F>;

    using namespace magic_enum::bitwise_operators;
    const rhi::RenderPassAccess default_screen_pass_access({ rhi::RenderPassAccess::Bit::ShaderResources, rhi::RenderPassAccess::Bit::Samplers });
    const rhi::ContextOptions default_context_options;
    const Graphics::DepthStencil        default_clear_depth_stencil(1.F, Graphics::Stencil(0));
    const Graphics::Color4F             default_clear_color(0.0F, 0.2F, 0.4F, 1.0F);

    return Graphics::CombinedAppSettings
    {                                                           // =========================
        Platform::AppSettings {                                 // platform_app:
            app_name,                                           //   - name
            { 0.8, 0.8 },                                       //   - size
            { 640U, 480U },                                     //   - min_size
            app_options.full_screen,                            //   - is_full_screen
            &Data::IconProvider::Get(),                         //   - icon_resources_ptr
        },                                                      // =========================
        Graphics::AppSettings {                                 // graphics_app:
            default_screen_pass_access,                         //   - screen_pass_access
            app_options.animations,                             //   - animations_enabled
            !app_options.hud_visible,                           //   - show_hud_in_window_title
            0                                                   //   - default_device_index
        },                                                      // =========================
        rhi::RenderContextSettings {                            // render_context:
            Graphics::FrameSize(),                              //   - frame_size
            Graphics::PixelFormat::BGRA8Unorm,                  //   - color_format
            app_options.depth_buffer                            //   - depth_stencil_format
                ? Graphics::PixelFormat::Depth32Float           //     ...
                : Graphics::PixelFormat::Unknown,               //     ...
            app_options.clear_color                             //   - clear_color
                ? ColorOpt(default_clear_color)                 //     ...
                : ColorOpt(),                                   //     ...
            app_options.depth_buffer && app_options.clear_depth //   - clear_depth_stencil
                ? DepthStencilOpt(default_clear_depth_stencil)  //     ...
                : DepthStencilOpt(),                            //     ...
            3U,                                                 //   - frame_buffers_count
            app_options.vsync,                                  //   - vsync_enabled
            app_options.full_screen,                            //   - is_full_screen
            default_context_options,                            //   - options_mask
            1000U,                                              //   - unsync_max_fps (MacOS only)
        }                                                       // =========================
    };
}

UserInterface::IApp::Settings GetUserInterfaceTutorialAppSettings(AppOptions app_options)
{
    return UserInterface::IApp::Settings
        {
            app_options.hud_visible ? UserInterface::HeadsUpDisplayMode::UserInterface
                                    : UserInterface::HeadsUpDisplayMode::WindowTitle,
            true // badge_visible
        };
}

} // namespace Methane::Tutorials
