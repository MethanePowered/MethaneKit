/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

#include <vector>
#include <cstdlib>
#include <cassert>

using namespace Methane;
using namespace Methane::Platform;

AppBase::AppBase(const AppBase::Settings& settings)
    : m_settings(settings)
    , m_cmd_options(GetExecutableFileName(), settings.name)
{
    m_cmd_options
        .allow_unrecognised_options()
        .add_options()
        ("help",     "Print command line options help")
        ("w,width",  "Window width in pixels or as ratio of desctop width",   cxxopts::value<double>())
        ("h,height", "Window height in pixels or as ratio of desctop height", cxxopts::value<double>());
}

void AppBase::ParseCommandLine(const cxxopts::ParseResult& cmd_parse_result)
{
    if (cmd_parse_result.count("help"))
    {
        const Message help_msg = {
            Message::Type::Information,
            "Command Line Options",
            m_cmd_options.help()
        };
        Alert(help_msg, true);
    }
    if (cmd_parse_result.count("width"))
    {
        m_settings.width = cmd_parse_result["width"].as<double>();
    }
    if (cmd_parse_result.count("height"))
    {
        m_settings.height = cmd_parse_result["height"].as<double>();
    }
}

int AppBase::Run(const RunArgs& args)
{
#ifdef __APPLE__
    // NOTE: MacOS bundle process serial number argument must be skipped because of unsupported syntax (underscores)
    const std::string macos_psn_arg_prefix = "-psn_";
#endif
    
    // Create a mutable copy of command line args to let the parser modify it
    int mutable_args_count = 0;
    std::vector<char*> mutable_arg_values(args.cmd_arg_count, nullptr);
    for (int argi = 0; argi < args.cmd_arg_count; argi++)
    {
        const size_t arg_value_size = strlen(args.cmd_arg_values[argi]) + 1;
#ifdef __APPLE__
        if (strncmp(args.cmd_arg_values[argi], macos_psn_arg_prefix.data(), std::min(arg_value_size - 1, macos_psn_arg_prefix.length())) == 0)
            continue;
#endif
        mutable_arg_values[mutable_args_count] = new char[arg_value_size];
#ifdef _WIN32
        strcpy_s(mutable_arg_values[argi], arg_value_size, args.cmd_arg_values[argi]);
#else
        strlcpy(mutable_arg_values[argi], args.cmd_arg_values[argi], arg_value_size);
#endif
        mutable_args_count++;
    };

    // Parse command line
    int return_code = 0;
    try
    {
        char** p_mutable_arg_values = mutable_arg_values.data();
        const cxxopts::ParseResult cmd_parse_result = m_cmd_options.parse(mutable_args_count, p_mutable_arg_values);
        ParseCommandLine(cmd_parse_result);
    }
    catch (const cxxopts::OptionException& e)
    {
        const Message help_msg = {
            Message::Type::Error,
            "Command Line Parse Error",
            std::string("Failed to parse command line option: ") + e.what()
        };
        Alert(help_msg);
        return_code = 1;
    }

    // Delete mutable args
    for (char* p_arg_value : mutable_arg_values)
    {
        delete[](p_arg_value);
    };

    return return_code;
}

void AppBase::Init()
{
    m_initialized = true;
}

void AppBase::KeyboardChanged(Keyboard::Key key, Keyboard::KeyState key_state)
{
    Keyboard::State prev_keyboard_state(m_keyboard_state);
    m_keyboard_state.SetKey(key, key_state);
    if (m_keyboard_state != prev_keyboard_state)
    {
        OnKeyboardStateChanged(m_keyboard_state);
    }
}

void AppBase::MouseButtonsChanged(Mouse::Button button, Mouse::ButtonState button_state)
{
    Mouse::State prev_mouse_state(m_mouse_state);
    m_mouse_state.SetButton(button, button_state);
    if (m_mouse_state != prev_mouse_state)
    {
        OnMouseStateChanged(m_mouse_state);
    }
}

void AppBase::MousePositionChanged(Mouse::Position mouse_position)
{
    Mouse::State prev_mouse_state(m_mouse_state);
    m_mouse_state.SetPosition(mouse_position);
    if (m_mouse_state != prev_mouse_state)
    {
        OnMouseStateChanged(m_mouse_state);
    }
}

void AppBase::MouseInWindowChanged(bool is_mouse_in_window)
{
    if (m_mouse_in_window != is_mouse_in_window)
    {
        m_mouse_in_window = is_mouse_in_window;
        OnMouseStateChanged(m_mouse_state);
    }
}

void AppBase::OnKeyboardStateChanged(const Keyboard::State& keyboard_state)
{
    PrintToDebugOutput(std::string("Keyboard: ") + keyboard_state.ToString());
}

void AppBase::OnMouseStateChanged(const Mouse::State& mouse_state)
{
    PrintToDebugOutput(std::string(m_mouse_in_window ? "Mouse in window: " : "Mouse out of window: ") + mouse_state.ToString());
}

void AppBase::ChangeWindowBounds(const Data::FrameRect& window_bounds)
{
    m_window_bounds = window_bounds;
}

void AppBase::Alert(const Message& msg, bool deferred)
{
    if (!deferred)
        return;

    m_sp_deferred_message.reset(new Message(msg));
}

bool AppBase::HasError() const
{
    return m_sp_deferred_message ? m_sp_deferred_message->type == Message::Type::Error : false;
}
