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
#ifdef APPLE_IOS
    options |= AppOptions::Bit::FullScreen;
    options |= AppOptions::Bit::HudVisible;
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
    constexpr rhi::ContextOptionMask default_context_options{ rhi::ContextOption::DeferredProgramBindingsInitialization };
    const DepthStencilValues         default_clear_depth_stencil(1.F, Graphics::Stencil(0));
    const Color4F                    default_clear_color(0.0F, 0.2F, 0.4F, 1.0F);

    using enum AppOptions::Bit;
    return Graphics::CombinedAppSettings
    {
        .platform_app = Platform::AppSettings {
            .name = app_name,
            .size = { 0.8, 0.8 },
            .min_size = { 640U, 480U },
            .is_full_screen = app_options.HasBit(FullScreen),
            .icon_provider_ptr = &Data::IconProvider::Get(),
        },
        .graphics_app = Graphics::AppSettings {
            .screen_pass_access = default_screen_pass_access,
            .animations_enabled = app_options.HasBit(Animations),
            .show_hud_in_window_title = !app_options.HasBit(HudVisible),
            .default_device_index = 0
        },
        .render_context = rhi::RenderContextSettings {
            .frame_size = Graphics::FrameSize(),
            .color_format = Graphics::PixelFormat::BGRA8Unorm,
            .depth_stencil_format = app_options.HasBit(DepthBuffer)
                ? Graphics::PixelFormat::Depth32Float
                : Graphics::PixelFormat::Unknown,
            .clear_color = app_options.HasBit(ClearColor)
                ? Opt<Color4F>(default_clear_color)
                : Opt<Color4F>(),
            .clear_depth_stencil = app_options.HasBits({ DepthBuffer, ClearDepth })
                ? Opt<DepthStencilValues>(default_clear_depth_stencil)
                : Opt<DepthStencilValues>(),
            .frame_buffers_count = 3U,
            .vsync_enabled = app_options.HasBit(VSync),
            .is_full_screen = app_options.HasBit(FullScreen),
            .options_mask = default_context_options,
            .unsync_max_fps = 1000U, // MacOS only
        }
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
