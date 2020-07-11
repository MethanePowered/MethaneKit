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

FILE: Methane/Platform/AppBase.h
Base application interface and platform-independent implementation.

******************************************************************************/

#pragma once

#include <Methane/Platform/AppView.h>
#include <Methane/Platform/Input/State.h>
#include <Methane/Data/Types.h>
#include <Methane/Memory.hpp>
#include <Methane/Instrumentation.h>

#include <CLI/App.hpp>

#include <string>
#include <vector>
#include <memory>

namespace Methane::Platform
{

struct AppEnvironment;

class AppBase : public CLI::App
{
public:
    struct Settings
    {
        std::string name;
        double      width          = 0.8;   // if width < 1.0 use as ratio of desktop size; else use as exact size in pixels/dots
        double      height         = 0.8;   // same rule applies for height
        bool        is_full_screen = false;
        uint32_t    min_width      = 640;
        uint32_t    min_height     = 480;
    };

    struct RunArgs
    {
        int          cmd_arg_count;
        const char** cmd_arg_values;
    };
    
    struct Message
    {
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
    virtual void ChangeWindowBounds(const Data::FrameRect& window_bounds);
    virtual void StartResizing();
    virtual void EndResizing();
    virtual bool Resize(const Data::FrameSize& frame_size, bool is_minimized);
    virtual bool Update() = 0;
    virtual bool Render() = 0;
    virtual void Alert(const Message& msg, bool deferred = false);
    virtual void SetWindowTitle(const std::string& title_text) = 0;
    virtual bool SetFullScreen(bool is_full_screen);
    virtual void ShowControlsHelp();
    virtual void ShowCommandLineHelp();
    virtual void ShowParameters() { }
    virtual void Close() = 0;

    void UpdateAndRender();
    bool HasError() const;

    const Settings&         GetPlatformAppSettings() const  { return m_settings; }
    const Input::State&     GetInputState() const           { return m_input_state; }
    const Data::FrameSize&  GetFrameSize() const            { return m_frame_size; }
    bool                    IsMinimized() const             { return m_is_minimized; }
    bool                    IsResizing() const              { return m_is_resizing; }

    template<typename FuncType, typename... ArgTypes>
    void ProcessInput(FuncType&& func_ptr, ArgTypes&&... args)
    {
        META_FUNCTION_TASK();
#ifndef _DEBUG
        try
        {
#endif
            (m_input_state.*std::forward<FuncType>(func_ptr))(std::forward<ArgTypes>(args)...);
#ifndef _DEBUG
        }
        catch (std::exception& e)
        {
            Alert({ Message::Type::Error, "Application Input Error", e.what() });
        }
        catch (...)
        {
            Alert({ Message::Type::Error, "Application Input Error", "Unknown exception occurred." });
        }
#endif
    }

protected:
    // AppBase interface
    virtual AppView GetView() const = 0;
    virtual void ShowAlert(const Message& msg);

    std::string GetControlsHelp();
    std::string GetCommandLineHelp() { return CLI::App::help(); }

    void AddInputControllers(const Ptrs<Input::Controller>& controllers) { m_input_state.AddControllers(controllers); }
    void Deinitialize() { m_initialized = false; }

    Ptr<Message> m_sp_deferred_message;

private:
    Settings             m_settings;
    Data::FrameRect      m_window_bounds;
    Data::FrameSize      m_frame_size;
    bool                 m_is_minimized = false;
    bool                 m_initialized = false;
    bool                 m_is_resizing = false;
    Input::State         m_input_state;
};

} // namespace Methane::Platform
