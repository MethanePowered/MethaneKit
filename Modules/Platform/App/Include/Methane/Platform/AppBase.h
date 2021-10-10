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

#include "IApp.h"

#include <Methane/Platform/AppView.h>
#include <Methane/Platform/Input/State.h>
#include <Methane/Memory.hpp>
#include <Methane/Instrumentation.h>

#include <CLI/App.hpp>

#include <fmt/format.h>

namespace tf // NOSONAR
{
// TaskFlow Executor class forward declaration:
// #include <taskflow/core/executor.hpp>
class Executor;
}

namespace Methane::Platform
{

class AppBase
    : public CLI::App
    , public IApp
{
public:
    explicit AppBase(const Settings& settings);
    ~AppBase() override;

    // IApp overrides
    int  Run(const RunArgs& args) override;
    void Init() override;
    void ChangeWindowBounds(const Data::FrameRect& window_bounds) override;
    void StartResizing() override;
    void EndResizing() override;
    bool Resize(const Data::FrameSize& frame_size, bool is_minimized) override;
    void Alert(const Message& msg, bool deferred = false) override;
    bool SetFullScreen(bool is_full_screen) override;
    bool SetKeyboardFocus(bool has_keyboard_focus) override;
    void ShowControlsHelp() override;
    void ShowCommandLineHelp() override;
    void ShowParameters() override { /* no parameters are displayed by default, but can be overridden */ }

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

    bool HasDeferredMessage() const noexcept { return !!m_deferred_message_ptr; }
    const Message& GetDeferredMessage() const;
    void ResetDeferredMessage() noexcept { m_deferred_message_ptr.reset(); }

    template<typename ScalarType>
    static ScalarType GetScaledSize(float scaled_size, ScalarType full_size)
    {
        return static_cast<ScalarType>(scaled_size < 1.F ? scaled_size * static_cast<float>(full_size) : scaled_size);
    }

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
    Ptr<Message>    m_deferred_message_ptr;
    bool            m_is_minimized = false;
    bool            m_initialized = false;
    bool            m_is_resizing = false;
    bool            m_has_keyboard_focus = false;
    Input::State    m_input_state;

    mutable UniquePtr<tf::Executor> m_parallel_executor_ptr;
};

} // namespace Methane::Platform
