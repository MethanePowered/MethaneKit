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

#include <vector>
#include <cstdlib>
#include <cassert>

namespace Methane::Platform
{

AppBase::AppBase(const AppBase::Settings& settings)
    : m_settings(settings)
    , m_cmd_options(GetExecutableFileName(), settings.name)
{
    ITT_FUNCTION_TASK();
    m_cmd_options
        .allow_unrecognised_options()
        .add_options()
        ("help",     "Print command line options help")
        ("w,width",  "Window width in pixels or as ratio of desctop width",   cxxopts::value<double>())
        ("h,height", "Window height in pixels or as ratio of desctop height", cxxopts::value<double>());
}

void AppBase::ParseCommandLine(const cxxopts::ParseResult& cmd_parse_result)
{
    ITT_FUNCTION_TASK();
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
    ITT_FUNCTION_TASK();

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
#elif defined __APPLE__
        strlcpy(mutable_arg_values[argi], args.cmd_arg_values[argi], arg_value_size);
#elif defined __linux__
        strcpy(mutable_arg_values[argi], args.cmd_arg_values[argi]);
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
    // We asssume that user has released all previously pressed keys and simulate these events
    m_input_state.ReleaseAllKeys();
}

void AppBase::UpdateAndRender()
{
    // Do not render if error has occured and is being displayed in message box
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
