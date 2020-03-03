/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Platform/AppBase.cpp
Base application interface and platform-independent implementation.

******************************************************************************/

#include <Methane/Platform/AppBase.h>
#include <Methane/Platform/Utils.h>
#include <Methane/Instrumentation.h>

#include <CLI/CLI.hpp>

#include <vector>
#include <cstdlib>
#include <cassert>

namespace Methane::Platform
{

AppBase::AppBase(const AppBase::Settings& settings)
    : CLI::App(settings.name, GetExecutableFileName())
    , m_settings(settings)
{
    ITT_FUNCTION_TASK();

    add_option("-w,--width",  m_settings.width,  "Window width in pixels or as ratio of desktop width", true);
    add_option("-x,--height", m_settings.height, "Window height in pixels or as ratio of desktop height", true);
    add_option("-f,--full-screen", m_settings.is_full_screen, "Full-screen mode", true);

#ifdef __APPLE__
    // When application is opened on MacOS with its Bundle,
    // OS adds an additional command-line option which looks like "-psn_0_23004655" which should be allowed
    allow_extras();
#endif
}

int AppBase::Run(const RunArgs& args)
{
    ITT_FUNCTION_TASK();

    try
    {
        parse(args.cmd_arg_count, args.cmd_arg_values);
    }
    catch(const CLI::CallForHelp&)
    {
        Alert(Message{
            Message::Type::Information,
            "Command Line Options",
            help()
        }, true);
    }
    catch(const CLI::ParseError& e)
    {
        Alert(Message{
            Message::Type::Error,
            "Command Line Parse Error",
            std::string("Failed to parse command line: ") + e.what()
        });
        return exit(e);
    }

    return 0;
}

void AppBase::Init()
{
    ITT_FUNCTION_TASK();
    m_initialized = true;
}

void AppBase::ChangeWindowBounds(const Data::FrameRect& window_bounds)
{
    ITT_FUNCTION_TASK();
    m_window_bounds = window_bounds;
}
    
bool AppBase::Resize(const Data::FrameSize& frame_size, bool is_minimized)
{
    ITT_FUNCTION_TASK();
    
    const bool is_resizing = !is_minimized && m_frame_size != frame_size;
    
    m_is_minimized = is_minimized;
    if (!m_is_minimized)
    {
        m_frame_size = frame_size;
    }
    
    return m_initialized && is_resizing;
}

void AppBase::Alert(const Message& msg, bool deferred)
{
    ITT_FUNCTION_TASK();
    if (!deferred)
        return;

    m_sp_deferred_message.reset(new Message(msg));
}

void AppBase::ShowAlert(const Message&)
{
    ITT_FUNCTION_TASK();

    // Message box interrupts message loop so that application looses all key release events
    // We assume that user has released all previously pressed keys and simulate these events
    m_input_state.ReleaseAllKeys();
}

void AppBase::UpdateAndRender()
{
    // Do not render if error has occurred and is being displayed in message box
    if (HasError())
        return;

    Update();
    Render();
}

bool AppBase::HasError() const
{
    ITT_FUNCTION_TASK();
    return m_sp_deferred_message ? m_sp_deferred_message->type == Message::Type::Error : false;
}

bool AppBase::SetFullScreen(bool is_full_screen)
{
    ITT_FUNCTION_TASK();
    if (m_settings.is_full_screen == is_full_screen)
        return false;

    m_settings.is_full_screen = is_full_screen;
    return true;
}

} // namespace Methane::Platform
