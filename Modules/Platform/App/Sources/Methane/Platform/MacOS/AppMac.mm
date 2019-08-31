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

FILE: Methane/Platform/MacOS/AppMac.mm
MacOS application implementation.

******************************************************************************/

#include <Methane/Platform/MacOS/AppMac.hh>
#include <Methane/Platform/MacOS/Types.hh>

using namespace Methane::Platform;
using namespace Methane::MacOS;

NSAlertStyle ConvertMessageTypeToNSAlertStyle(AppBase::Message::Type msg_type)
{
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
    , m_ns_app_delegate([[AppDelegate alloc] initWithApp:this andSettings: &m_settings])
{
    [m_ns_app setDelegate: m_ns_app_delegate];
}

AppMac::~AppMac()
{
    [m_ns_app_delegate release];
    [m_ns_app release];
}

void AppMac::InitContext(const Platform::AppEnvironment& /*env*/, const Data::FrameSize& /*frame_size*/)
{
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
    const int base_return_code = AppBase::Run(args);
    if (base_return_code)
        return base_return_code;

    [m_ns_app_delegate run];
    [m_ns_app run];
    return 0;
}

void AppMac::Alert(const Message& msg, bool deferred)
{
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
    NSString* ns_title_text = ConvertToNSType<std::string, NSString*>(title_text);
    dispatch_async(dispatch_get_main_queue(), ^(void){
        m_ns_window.title = ns_title_text;
    });
}

void AppMac::SetWindow(NSWindow* ns_window)
{
    m_ns_window = ns_window;
    
}

void AppMac::ShowAlert(const Message& msg)
{
    assert(m_ns_app_delegate);
    [m_ns_app_delegate alert: ConvertToNSType<std::string, NSString*>(msg.title)
             withInformation: ConvertToNSType<std::string, NSString*>(msg.information)
                    andStyle: ConvertMessageTypeToNSAlertStyle(msg.type)];

    AppBase::ShowAlert(msg);
}

bool AppMac::SetFullScreen(bool is_full_screen)
{
    if (!AppBase::SetFullScreen(is_full_screen))
        return false;
    
    if (m_ns_window)
    {
        [m_ns_window toggleFullScreen:nil];
    }
    return true;
}
