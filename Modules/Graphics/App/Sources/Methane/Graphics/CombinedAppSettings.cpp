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

FILE: Methane/Graphics/AppBase.h
Graphics and Platform application combined settings.

******************************************************************************/

#include <Methane/Graphics/CombinedAppSettings.h>

namespace Methane::Graphics
{

CombinedAppSettings& CombinedAppSettings::SetPlatformAppSettings(Platform::AppSettings&& new_platform_app_settings) noexcept
{
    META_FUNCTION_TASK();
    platform_app = std::move(new_platform_app_settings);
    return *this;
}

CombinedAppSettings& CombinedAppSettings::SetGraphicsAppSettings(Graphics::AppSettings&& new_graphics_app_settings) noexcept
{
    META_FUNCTION_TASK();
    graphics_app = std::move(new_graphics_app_settings);
    return *this;
}

CombinedAppSettings& CombinedAppSettings::SetRenderContextSettings(Rhi::RenderContextSettings&& new_render_context_settings) noexcept
{
    META_FUNCTION_TASK();
    render_context = std::move(new_render_context_settings);
    return *this;
}

} // namespace Methane::Graphics