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

namespace Methane::Tutorials
{

namespace rhi = Methane::Graphics::Rhi;

AppOptions::Mask AppOptions::GetDefaultWithColorOnly() noexcept
{
    AppOptions::Mask options;
    options |= AppOptions::Bit::ClearColor;
#ifdef __APPLE__
    options |= AppOptions::Bit::VSync;
#ifndef APPLE_MACOS // iOS
    options |= AppOptions::Bit::FullScreen;
    options |= AppOptions::Bit::HudVisible;
#endif
#endif
    return options;
}

AppOptions::Mask AppOptions::GetDefaultWithColorDepth() noexcept
{
    AppOptions::Mask options(GetDefaultWithColorOnly());
    options |= AppOptions::Bit::DepthBuffer;
    options |= AppOptions::Bit::ClearDepth;
    return options;
}

AppOptions::Mask AppOptions::GetDefaultWithColorDepthAndAnim() noexcept
{
    AppOptions::Mask options(GetDefaultWithColorDepth());
    options |= AppOptions::Bit::Animations;
    return options;
}

AppOptions::Mask AppOptions::GetDefaultWithColorOnlyAndAnim() noexcept
{
    AppOptions::Mask options(GetDefaultWithColorOnly());
    options |= AppOptions::Bit::Animations;
    return options;
}

Graphics::CombinedAppSettings GetGraphicsTutorialAppSettings(const std::string& app_name, AppOptions::Mask app_options)
{
    using namespace Methane::Graphics;

    constexpr rhi::RenderPassAccessMask default_screen_pass_access({ rhi::RenderPassAccess::ShaderResources, rhi::RenderPassAccess::Samplers });
    constexpr rhi::ContextOptionMask    default_context_options;
    const DepthStencil                  default_clear_depth_stencil(1.F, Graphics::Stencil(0));
    const Color4F                       default_clear_color(0.0F, 0.2F, 0.4F, 1.0F);

    return Graphics::CombinedAppSettings
    {                                                           // =========================
        Platform::AppSettings {                                 // platform_app:
            app_name,                                           //   - name
            { 0.8, 0.8 },                                       //   - size
            { 640U, 480U },                                     //   - min_size
            app_options.HasBit(AppOptions::Bit::FullScreen),    //   - is_full_screen
            &Data::IconProvider::Get(),                         //   - icon_resources_ptr
        },                                                      // =========================
        Graphics::AppSettings {                                 // graphics_app:
            default_screen_pass_access,                         //   - screen_pass_access
            app_options.HasBit(AppOptions::Bit::Animations),    //   - animations_enabled
            !app_options.HasBit(AppOptions::Bit::HudVisible),   //   - show_hud_in_window_title
            0                                                   //   - default_device_index
        },                                                      // =========================
        rhi::RenderContextSettings {                            // render_context:
            Graphics::FrameSize(),                              //   - frame_size
            Graphics::PixelFormat::BGRA8Unorm,                  //   - color_format
            app_options.HasBit(AppOptions::Bit::DepthBuffer)    //   - depth_stencil_format
                ? Graphics::PixelFormat::Depth32Float           //     ...
                : Graphics::PixelFormat::Unknown,               //     ...
            app_options.HasBit(AppOptions::Bit::ClearColor)     //   - clear_color
                ? Opt<Color4F>(default_clear_color)             //     ...
                : Opt<Color4F>(),                               //     ...
            app_options.HasBits({ AppOptions::Bit::DepthBuffer, //   - clear_depth_stencil
                                  AppOptions::Bit::ClearDepth })//     ...
                ? Opt<DepthStencil>(default_clear_depth_stencil)//     ...
                : Opt<DepthStencil>(),                          //     ...
            3U,                                                 //   - frame_buffers_count
            app_options.HasBit(AppOptions::Bit::VSync),         //   - vsync_enabled
            app_options.HasBit(AppOptions::Bit::FullScreen),    //   - is_full_screen
            default_context_options,                            //   - options_mask
            1000U,                                              //   - unsync_max_fps (MacOS only)
        }                                                       // =========================
    };
}

UserInterface::IApp::Settings GetUserInterfaceTutorialAppSettings(AppOptions::Mask app_options)
{
    return UserInterface::IApp::Settings
    {
        app_options.HasBit(AppOptions::Bit::HudVisible)
            ? UserInterface::HeadsUpDisplayMode::UserInterface
            : UserInterface::HeadsUpDisplayMode::WindowTitle,
        true // badge_visible
    };
}

} // namespace Methane::Tutorials
