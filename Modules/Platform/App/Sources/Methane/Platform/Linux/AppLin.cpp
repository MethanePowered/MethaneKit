/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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

FILE: Methane/Platform/Linux/AppLin.cpp
Linux application implementation.

******************************************************************************/

#include <Methane/Platform/Linux/AppLin.h>
#include <Methane/Platform/Utils.h>
#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>

#include <xcb/xcb.h>

namespace Methane::Platform
{

static xcb_connection_t* XcbConnect(const char* display_name = nullptr, int* screen = nullptr)
{
    META_FUNCTION_TASK();
    xcb_connection_t* xcb_connection = ::xcb_connect(display_name, screen);
    const int xcb_connection_error = ::xcb_connection_has_error(xcb_connection);
    META_CHECK_ARG_EQUAL_DESCR(xcb_connection_error, 0, "XCB connection to display has failed");
    return xcb_connection;
}

AppLin::AppLin(const AppBase::Settings& settings)
    : AppBase(settings)
    , m_xcb_connection(XcbConnect())
{
    META_FUNCTION_TASK();
}

AppLin::~AppLin()
{
    META_FUNCTION_TASK();
    xcb_disconnect(m_xcb_connection);
}

int AppLin::Run(const RunArgs& /*args*/)
{
    return 0;
}

void AppLin::Alert(const Message& msg, bool deferred)
{
    META_FUNCTION_TASK();
    AppBase::Alert(msg, deferred);
}

void AppLin::ShowAlert(const Message& /*msg*/)
{
    META_FUNCTION_TASK();
}

void AppLin::SetWindowTitle(const std::string& /*title_text*/)
{
    META_FUNCTION_TASK();
}

bool AppLin::SetFullScreen(bool is_full_screen)
{
    META_FUNCTION_TASK();
    if (!AppBase::SetFullScreen(is_full_screen))
        return false;

    return true;
}

void AppLin::Close()
{
    META_FUNCTION_TASK();
}

} // namespace Methane::Platform