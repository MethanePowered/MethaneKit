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

#include <fmt/format.h>
#include <string>
#include <vector>
#include <memory>

namespace tf
{
// TaskFlow Executor class forward declaration:
// #include <taskflow/core/executor.hpp>
class Executor;
}

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

    explicit AppBase(const Settings& settings);
    ~AppBase() override;

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
    virtual bool SetKeyboardFocus(bool has_keyboard_focus);
    virtual void ShowControlsHelp();
    virtual void ShowCommandLineHelp();
    virtual void ShowParameters() { }
    virtual void Close() = 0;

    bool InitContextWithErrorHandling(const Platform::AppEnvironment& env, const Data::FrameSize& frame_size)
    { return ExecuteWithErrorHandling("Render Context Initialization", *this, &AppBase::InitContext, env, frame_size); }

    bool InitWithErrorHandling()            { return ExecuteWithErrorHandling("Application Initialization", *this, &AppBase::Init); }
    bool UpdateAndRenderWithErrorHandling() { return ExecuteWithErrorHandling("Application Rendering", *this, &AppBase::UpdateAndRender); }

    template<typename FuncType, typename... ArgTypes>
    void ProcessInputWithErrorHandling(FuncType&& func_ptr, ArgTypes&&... args)
    { ExecuteWithErrorHandling("Application Input", m_input_state, std::forward<FuncType>(func_ptr), std::forward<ArgTypes>(args)...); }

    tf::Executor&           GetParallelExecutor() const;
    const Settings&         GetPlatformAppSettings() const noexcept { return m_settings; }
    const Input::State&     GetInputState() const noexcept          { return m_input_state; }
    const Data::FrameSize&  GetFrameSize() const noexcept           { return m_frame_size; }
    bool                    IsMinimized() const noexcept            { return m_is_minimized; }
    bool                    IsResizing() const noexcept             { return m_is_resizing; }
    bool                    HasKeyboardFocus() const noexcept       { return m_has_keyboard_focus; }
    bool                    HasError() const noexcept;

protected:
    // AppBase interface
    virtual AppView GetView() const = 0;
    virtual void ShowAlert(const Message& msg);

    std::string GetControlsHelp() const;
    std::string GetCommandLineHelp() const { return CLI::App::help(); }

    void AddInputControllers(const Ptrs<Input::Controller>& controllers) { m_input_state.AddControllers(controllers); }
    void Deinitialize() { m_initialized = false; }

    Ptr<Message> m_deferred_message_ptr;

private:
    bool UpdateAndRender();

    template<typename ObjectType, typename FuncType, typename... ArgTypes>
    bool ExecuteWithErrorHandling(const char* stage_name, ObjectType& obj, FuncType&& func_ptr, ArgTypes&&... args)
#ifdef _DEBUG
        const
#endif
    {
        // We do not catch exceptions in Debug build to let them be handled by the Debugger
#ifndef _DEBUG
        try
        {
#else
        META_UNUSED(stage_name);
#endif
            (obj.*std::forward<FuncType>(func_ptr))(std::forward<ArgTypes>(args)...);
#ifndef _DEBUG
        }
        catch (std::exception& e)
        {
            Alert({ Message::Type::Error, fmt::format("{} Error", stage_name), e.what() });
            return false;
        }
        catch (...)
        {
            Alert({ Message::Type::Error, fmt::format("{} Error", stage_name), "Unknown exception occurred." });
            return false;
        }
#endif
        return true;
    }

    Settings        m_settings;
    Data::FrameRect m_window_bounds;
    Data::FrameSize m_frame_size;
    bool            m_is_minimized = false;
    bool            m_initialized = false;
    bool            m_is_resizing = false;
    bool            m_has_keyboard_focus = false;
    Input::State    m_input_state;

    mutable UniquePtr<tf::Executor> m_parallel_executor_ptr;
};

} // namespace Methane::Platform
