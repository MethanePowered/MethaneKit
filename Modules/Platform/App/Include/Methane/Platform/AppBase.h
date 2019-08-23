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

FILE: Methane/Platform/AppBase.h
Base application interface and platform-independent implementation.

******************************************************************************/

#pragma once

#include <Methane/Platform/AppView.h>
#include <Methane/Platform/Input/State.h>
#include <Methane/Data/Types.h>

#include <cxxopts.hpp>

#include <string>
#include <vector>
#include <memory>

namespace Methane
{
namespace Platform
{

struct AppEnvironment;

class AppBase
{
public:
    struct Settings
    {
        std::string name;
        double      width;   // if width < 1.0 use as ratio of desktop size; else use as exact size in pixels/dots
        double      height;  // same rule applies for height
    };

    struct RunArgs
    {
        int          cmd_arg_count;
        const char** cmd_arg_values;
    };
    
    struct Message
    {
        using Ptr = std::unique_ptr<Message>;

        enum class Type : uint32_t
        {
            Information = 0,
            Warning,
            Error
        };
        
        Type        type;
        std::string title;
        std::string information;
    };

    AppBase(const Settings& settings);
    virtual ~AppBase() = default;

    // AppBase interface
    virtual int  Run(const RunArgs& args);
    virtual void InitContext(const Platform::AppEnvironment& env, const Data::FrameSize& frame_size) = 0;
    virtual void Init();
    virtual bool Resize(const Data::FrameSize& frame, bool is_minimized) = 0;
    virtual void ChangeWindowBounds(const Data::FrameRect& window_bounds);
    virtual void Update() = 0;
    virtual void Render() = 0;
    virtual void Alert(const Message& msg, bool deferred = false);
    virtual void SetWindowTitle(const std::string& title_text) = 0;

    bool HasError() const;

    const Settings&         GetSettings() const   { return m_settings; }
    const Input::State&     GetInputState() const { return m_input_state; }
    const cxxopts::Options& GetCmdOptions() const { return m_cmd_options; }

    // Entry point for user input handling from platform-specific implementation
    Input::IActionController& InputController() { return m_input_state; }

protected:
    // AppBase interface
    virtual AppView GetView() const = 0;
    virtual void ParseCommandLine(const cxxopts::ParseResult& cmd_parse_result);
    virtual void ShowAlert(const Message& msg);

    Settings             m_settings;
    Data::FrameRect      m_window_bounds;
    cxxopts::Options     m_cmd_options;
    bool                 m_initialized = false;
    Message::Ptr         m_sp_deferred_message;
    Input::State         m_input_state;
};

} // namespace Platform
} // namespace Methane
