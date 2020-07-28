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
#include <Methane/Platform/Logger.h>
#include <Methane/Platform/Input/Controller.h>
#include <Methane/ScopeTimer.h>
#include <Methane/Instrumentation.h>
#include <Methane/Version.h>

#include <CLI/CLI.hpp>
#include <taskflow/core/executor.hpp>

#include <sstream>
#include <vector>
#include <cstdlib>
#include <cassert>

namespace Methane::Platform
{

AppBase::AppBase(const AppBase::Settings& settings)
    : CLI::App(settings.name, GetExecutableFileName())
    , m_settings(settings)
{
    META_THREAD_NAME("Main Thread");
    META_FUNCTION_TASK();
    META_SCOPE_TIMERS_INITIALIZE(Methane::Platform::Logger);

    add_option("-w,--width",  m_settings.width,  "Window width in pixels or as ratio of desktop width", true);
    add_option("-x,--height", m_settings.height, "Window height in pixels or as ratio of desktop height", true);
    add_option("-f,--full-screen", m_settings.is_full_screen, "Full-screen mode", true);

#ifdef __APPLE__
    // When application is opened on MacOS with its Bundle,
    // OS adds an additional command-line option which looks like "-psn_0_23004655" which should be allowed
    allow_extras();
#endif
}

AppBase::~AppBase() = default;

int AppBase::Run(const RunArgs& args)
{
    META_FUNCTION_TASK();

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
    META_FUNCTION_TASK();
    m_initialized = true;
}

void AppBase::ChangeWindowBounds(const Data::FrameRect& window_bounds)
{
    META_FUNCTION_TASK();
    m_window_bounds = window_bounds;
}

void AppBase::StartResizing()
{
    META_FUNCTION_TASK();
    assert(!m_is_resizing);
    m_is_resizing = true;
}

void AppBase::EndResizing()
{
    META_FUNCTION_TASK();
    assert(m_is_resizing);
    m_is_resizing = false;
}
    
bool AppBase::Resize(const Data::FrameSize& frame_size, bool is_minimized)
{
    META_FUNCTION_TASK();
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
    META_FUNCTION_TASK();
    if (!deferred)
        return;

    m_sp_deferred_message.reset(new Message(msg));
}

void AppBase::ShowAlert(const Message&)
{
    META_FUNCTION_TASK();

    // Message box interrupts message loop so that application looses all key release events
    // We assume that user has released all previously pressed keys and simulate these events
    m_input_state.ReleaseAllKeys();
}

void AppBase::UpdateAndRender()
{
    META_FUNCTION_TASK();
    // Do not render if error has occurred and is being displayed in message box
    if (HasError())
        return;

    Update();
    Render();
}

bool AppBase::HasError() const
{
    META_FUNCTION_TASK();
    return m_sp_deferred_message ? m_sp_deferred_message->type == Message::Type::Error : false;
}

tf::Executor& AppBase::GetParallelExecutor() const
{
    META_FUNCTION_TASK();
    if (!m_sp_parallel_executor)
        m_sp_parallel_executor = std::make_unique<tf::Executor>();

    return *m_sp_parallel_executor;
}

bool AppBase::SetFullScreen(bool is_full_screen)
{
    META_FUNCTION_TASK();
    if (m_settings.is_full_screen == is_full_screen)
        return false;

    m_settings.is_full_screen = is_full_screen;
    m_input_state.ReleaseAllKeys();

    return true;
}

bool AppBase::SetKeyboardFocus(bool has_keyboard_focus)
{
    META_FUNCTION_TASK();
    if (m_has_keyboard_focus == has_keyboard_focus)
        return false;

    m_has_keyboard_focus = has_keyboard_focus;
    m_input_state.ReleaseAllKeys();

    return true;
}

std::string AppBase::GetControlsHelp()
{
    META_FUNCTION_TASK();

    std::stringstream help_stream;
    std::string single_offset = "    ";
    bool is_first_controller = true;

    for (const Ptr<Input::Controller>& sp_controller : GetInputState().GetControllers())
    {
        assert(!!sp_controller);
        if (!sp_controller) continue;

        const Input::IHelpProvider::HelpLines help_lines = sp_controller->GetHelp();
        if (help_lines.empty()) continue;

        if (!is_first_controller)
            help_stream << std::endl;

        std::string controller_offset;
        if (!sp_controller->GetControllerName().empty())
        {
            if (!is_first_controller)
                help_stream << std::endl;

            help_stream << sp_controller->GetControllerName();
            controller_offset = single_offset;
        }

        is_first_controller = false;

        bool first_line = true;
        bool header_present = false;
        for (const Input::IHelpProvider::KeyDescription& key_description : help_lines)
        {
            if (key_description.first.empty())
            {
                help_stream << std::endl << std::endl << controller_offset << key_description.second << ":";
                header_present = true;
            }
            else
            {
                if (first_line && !header_present)
                {
                    help_stream << std::endl;
                }
                help_stream << std::endl << controller_offset;
                if (header_present)
                {
                    help_stream << single_offset;
                }
                help_stream << key_description.first;
                if (!key_description.second.empty())
                {
                    help_stream << " - " << key_description.second;
                }
            }
            first_line = false;
        }
    }

    if (!is_first_controller)
    {
        help_stream << std::endl;
    }
    help_stream << std::endl << "Powered by " << METHANE_PRODUCT_NAME <<" v" METHANE_VERSION_STR
                << std::endl << METHANE_PRODUCT_URL;
    return help_stream.str();
}

void AppBase::ShowControlsHelp()
{
    META_FUNCTION_TASK();
    Alert({
        AppBase::Message::Type::Information,
        "Application Controls Help",
        GetControlsHelp()
    });
}

void AppBase::ShowCommandLineHelp()
{
    META_FUNCTION_TASK();
    Alert({
        AppBase::Message::Type::Information,
        "Application Command-Line Help",
        GetCommandLineHelp()
    });
}

} // namespace Methane::Platform
