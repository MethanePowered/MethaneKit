/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Metal/RenderContextAppView.hh
Metal application view creation by render context settings (shared logic with Vulkan impl).

******************************************************************************/

#pragma once

#include "Types.hh"

#include <Methane/Graphics/IRenderContext.h>
#include <Methane/Instrumentation.h>

#ifdef APPLE_MACOS
#import <Methane/Platform/MacOS/AppViewMetal.hh>
#else
#import <Methane/Platform/iOS/AppViewMetal.hh>
#endif

#import <Methane/Platform/Apple/Types.hh>

namespace Methane::Graphics::Metal
{

inline AppViewMetal* CreateRenderContextAppView(const Platform::AppEnvironment& env, const RenderContextSettings& settings)
{
    META_FUNCTION_TASK();
    AppViewMetal* app_view = [[AppViewMetal alloc] initWithFrame: TypeConverter::CreateNSRect(settings.frame_size)
                                                       appWindow: env.ns_app_delegate.window
                                                     pixelFormat: TypeConverter::DataFormatToMetalPixelType(settings.color_format)
                                                   drawableCount: settings.frame_buffers_count
                                                    vsyncEnabled: Methane::MacOS::ConvertToNsType<bool, BOOL>(settings.vsync_enabled)
#ifdef APPLE_MACOS
                                           unsyncRefreshInterval: 1.0 / settings.unsync_max_fps
#endif
    ];

    // bind Metal view with application delegate
    app_view.delegate = env.ns_app_delegate;
    env.ns_app_delegate.view = app_view;

    return app_view;
}

inline AppViewMetal* CreateTemporaryAppView(const Platform::AppEnvironment& env)
{
    META_FUNCTION_TASK();
    AppViewMetal* app_view = [[AppViewMetal alloc] initWithFrame: MakeNativeRect(0.f, 0.f, 1.f, 1.f)
                                                       appWindow: env.ns_app_delegate.window
                                                     pixelFormat: MTLPixelFormatBGRA8Unorm
                                                   drawableCount: 3
                                                    vsyncEnabled: YES
#ifdef APPLE_MACOS
                                           unsyncRefreshInterval: 0.01
#endif
    ];

    // bind Metal view with application delegate
    app_view.delegate = env.ns_app_delegate;
    env.ns_app_delegate.view = app_view;

    return app_view;
}

} // namespace Methane::Graphics::Metal
