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

FILE: Methane/Platform/MacOS/AppMac.h
MacOS application implementation.

******************************************************************************/

#pragma once

#include <Methane/Platform/AppBase.h>
#include <Methane/Platform/MacOS/AppEnvironment.hh>

#ifdef __OBJC__

#import <AppKit/NSApplication.h>
#import <AppKit/NSWindow.h>

#ifdef METHANE_RENDER_APP
#import <Methane/Platform/MacOS/AppDelegate.hh>
using AppDelegateType = AppDelegate;
#else
using AppDelegateType = void;
#endif

#else // __OBJC__

using AppDelegateType = void;
using NSApplication = void;
using NSWindow = void;

#endif // __OBJC__

namespace Methane::Platform
{

class AppMac : public AppBase
{
public:
    static AppMac* GetInstance();
    
    explicit AppMac(const AppBase::Settings& settings);

    // AppBase interface
    void InitContext(const Platform::AppEnvironment& env, const Data::FrameSize& frame_size) override;
    int  Run(const RunArgs& args) override;
    void Alert(const Message& msg, bool deferred = false) override;
    void SetWindowTitle(const std::string& title_text) override;
    bool SetFullScreen(bool is_full_screen) override;
    float GetContentScalingFactor() const override;
    uint32_t GetFontResolutionDpi() const override;
    void Close() override;

    void SetWindow(NSWindow* ns_window);
    bool SetFullScreenInternal(bool is_full_screen) { return AppBase::SetFullScreen(is_full_screen); }
    NSWindow* GetWindow()                           { return m_ns_window; } // NOSONAR

protected:
    // AppBase interface
    void ShowAlert(const Message& msg) override;

private:
    NSApplication*   m_ns_app          = nullptr; // NOSONAR
    AppDelegateType* m_ns_app_delegate = nullptr; // NOSONAR
    NSWindow*        m_ns_window       = nullptr; // NOSONAR
    
    static AppMac* s_instance_ptr;
};

} // namespace Methane::Platform
