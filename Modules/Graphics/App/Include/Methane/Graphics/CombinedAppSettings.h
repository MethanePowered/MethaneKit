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

FILE: Methane/Graphics/CombinedAppSettings.h
Graphics and Platform application combined settings.

******************************************************************************/

#pragma once

#include "IApp.h"

#include <Methane/Platform/App.h>
#include <Methane/Graphics/RHI/IRenderContext.h>

namespace Methane::Graphics
{

struct CombinedAppSettings
{
    Platform::AppSettings platform_app;
    Graphics::AppSettings graphics_app;
    Rhi::RenderContextSettings render_context;

    CombinedAppSettings& SetPlatformAppSettings(Platform::IApp::Settings&& new_platform_app_settings) noexcept;
    CombinedAppSettings& SetGraphicsAppSettings(Graphics::AppSettings&& new_graphics_app_settings) noexcept;
    CombinedAppSettings& SetRenderContextSettings(Rhi::RenderContextSettings&& new_render_context_settings) noexcept;
};

} // namespace Methane::Graphics