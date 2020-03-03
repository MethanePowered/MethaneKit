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

FILE: Methane/Platform/MacOS/AppMac.h
MacOS application implementation.

******************************************************************************/

#pragma once

#include <Methane/Platform/AppBase.h>
#include <Methane/Platform/AppEnvironment.h>

#if defined(__OBJC__) && defined(METHANE_RENDER_APP)

#import <Methane/Platform/MacOS/AppDelegate.hh>
#import <AppKit/AppKit.h>

using NSApplicationType = NSApplication;
using AppDelegateType = AppDelegate;
using NSWindowType = NSWindow;

#else

using NSApplicationType = void;
using AppDelegateType = void;
using NSWindowType = void;

#endif

namespace Methane::Platform
{

class AppMac : public AppBase
{
public:
    AppMac(const AppBase::Settings& settings);
    ~AppMac() override;

    // AppBase interface
    void InitContext(const Platform::AppEnvironment& env, const Data::FrameSize& frame_size) override;
    int  Run(const RunArgs& args) override;
    void Alert(const Message& msg, bool deferred = false) override;
    void SetWindowTitle(const std::string& title_text) override;
    bool SetFullScreen(bool is_full_screen) override;
    void Close() override;

    void SetWindow(NSWindowType* ns_window);
    bool SetFullScreenInternal(bool is_full_screen) { return AppBase::SetFullScreen(is_full_screen); }
    NSWindowType* GetWindow()                       { return m_ns_window; }

protected:
    // AppBase interface
    void ShowAlert(const Message& msg) override;

    NSApplicationType*  m_ns_app;
    AppDelegateType*    m_ns_app_delegate;
    NSWindowType*       m_ns_window;
};

} // namespace Methane::Platform
