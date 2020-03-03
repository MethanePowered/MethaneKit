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

FILE: Methane/Platform/MacOS/AppMac.mm
MacOS application implementation.

******************************************************************************/

#include <Methane/Platform/MacOS/AppMac.hh>
#include <Methane/Platform/MacOS/Types.hh>
#include <Methane/Instrumentation.h>

using namespace Methane::Platform;
using namespace Methane::MacOS;

NSAlertStyle ConvertMessageTypeToNsAlertStyle(AppBase::Message::Type msg_type)
{
    ITT_FUNCTION_TASK();
    switch(msg_type)
    {
        case AppBase::Message::Type::Information: return NSAlertStyleInformational;
        case AppBase::Message::Type::Warning:     return NSAlertStyleWarning;
        case AppBase::Message::Type::Error:       return NSAlertStyleCritical;
    }
}

AppMac::AppMac(const AppBase::Settings& settings)
    : AppBase(settings)
    , m_ns_app([NSApplication sharedApplication])
    , m_ns_app_delegate([[AppDelegate alloc] initWithApp:this andSettings: &settings])
{
    ITT_FUNCTION_TASK();
    [m_ns_app setDelegate: m_ns_app_delegate];
}

AppMac::~AppMac()
{
    ITT_FUNCTION_TASK();
    [m_ns_app_delegate release];
    [m_ns_app release];
}

void AppMac::InitContext(const Platform::AppEnvironment& /*env*/, const Data::FrameSize& /*frame_size*/)
{
    ITT_FUNCTION_TASK();
    AppView app_view = GetView();
    if (app_view.p_native_view == nil)
    {
        throw std::runtime_error("Native app view can not be null.");
    }

    if (m_ns_window == nil)
    {
        throw std::runtime_error("NS Window was not set.");
    }
    
    [m_ns_window.contentView addSubview: app_view.p_native_view];
}

int AppMac::Run(const RunArgs& args)
{
    ITT_FUNCTION_TASK();
    const int base_return_code = AppBase::Run(args);
    if (base_return_code)
        return base_return_code;

    [m_ns_app_delegate run];
    [m_ns_app run];
    return 0;
}

void AppMac::Alert(const Message& msg, bool deferred)
{
    ITT_FUNCTION_TASK();
    AppBase::Alert(msg, deferred);

    if (deferred)
    {
        dispatch_async(dispatch_get_main_queue(), ^{
            if (!m_sp_deferred_message)
                return;
            
            ShowAlert(*m_sp_deferred_message);
            m_sp_deferred_message.reset();
        });
    }
    else
    {
        ShowAlert(msg);
    }
}

void AppMac::SetWindowTitle(const std::string& title_text)
{
    ITT_FUNCTION_TASK();
    NSString* ns_title_text = ConvertToNsType<std::string, NSString*>(title_text);
    dispatch_async(dispatch_get_main_queue(), ^(void){
        m_ns_window.title = ns_title_text;
    });
}

void AppMac::SetWindow(NSWindow* ns_window)
{
    ITT_FUNCTION_TASK();
    m_ns_window = ns_window;
}

void AppMac::ShowAlert(const Message& msg)
{
    ITT_FUNCTION_TASK();
    assert(m_ns_app_delegate);
    [m_ns_app_delegate alert: ConvertToNsType<std::string, NSString*>(msg.title)
             withInformation: ConvertToNsType<std::string, NSString*>(msg.information)
                    andStyle: ConvertMessageTypeToNsAlertStyle(msg.type)];

    AppBase::ShowAlert(msg);
}

bool AppMac::SetFullScreen(bool is_full_screen)
{
    ITT_FUNCTION_TASK();
    if (!AppBase::SetFullScreen(is_full_screen))
        return false;
    
    const NSApplicationPresentationOptions app_options = [m_ns_app presentationOptions];
    const bool is_app_fullscreen = (app_options & NSApplicationPresentationFullScreen);
    if (is_app_fullscreen != is_full_screen)
    {
        [m_ns_window toggleFullScreen:nil];
    }
    return true;
}

void AppMac::Close()
{
    ITT_FUNCTION_TASK();
    [m_ns_window close];
}
