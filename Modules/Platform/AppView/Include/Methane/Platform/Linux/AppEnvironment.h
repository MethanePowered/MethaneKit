/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Platform/Linux/AppEnvironment.h
Linux application environment.

******************************************************************************/

#pragma once

struct xcb_connection_t;
struct xcb_screen_t;
struct _XDisplay; // X11 display

#ifndef xcb_window_t
using xcb_window_t = uint32_t;
#endif

struct ScreenRect
{
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
};

namespace Methane::Platform
{

struct AppEnvironment
{
    xcb_connection_t* connection;
    xcb_window_t window;
    _XDisplay* display;
    xcb_screen_t* screen;
    ScreenRect primary_screen_rect;
};

} // namespace Methane::Platform