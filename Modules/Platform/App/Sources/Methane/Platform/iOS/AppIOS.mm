/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: Methane/Platform/iOS/AppMac.mm
iOS application implementation.

******************************************************************************/

#include <Methane/Platform/iOS/AppIOS.hh>
#include <Methane/Platform/Apple/Types.hh>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Platform
{

AppMac* AppMac::s_instance_ptr = nullptr;

static UIAlertActionStyle ConvertMessageTypeToNsAlertStyle(AppBase::Message::Type msg_type)
{
    META_FUNCTION_TASK();
    switch(msg_type)
    {
        case AppBase::Message::Type::Information: return UIAlertActionStyleDefault;
        case AppBase::Message::Type::Warning:     return UIAlertActionStyleCancel;
        case AppBase::Message::Type::Error:       return UIAlertActionStyleDestructive;
        default: META_UNEXPECTED_ARG_RETURN(msg_type, UIAlertActionStyleDestructive);
    }
}

AppMac* AppMac::GetInstance()
{
    META_FUNCTION_TASK();
    return s_instance_ptr;
}

AppMac::AppMac(const AppBase::Settings& settings)
    : AppBase(settings)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL_DESCR(s_instance_ptr, nullptr, "Application can have only one instance");
    s_instance_ptr = this;
}

void AppMac::InitContext(const Platform::AppEnvironment& /*env*/, const Data::FrameSize& /*frame_size*/)
{
    META_FUNCTION_TASK();
}

int AppMac::Run(const RunArgs& args)
{
    META_FUNCTION_TASK();
    const int base_return_code = AppBase::Run(args);
    if (base_return_code)
        return base_return_code;

    return UIApplicationMain(args.cmd_arg_count, const_cast<char* *>(args.cmd_arg_values), nil,
                             NSStringFromClass([AppDelegate class]));
}

void AppMac::Alert(const Message& msg, bool deferred)
{
    META_FUNCTION_TASK();
    if (deferred)
    {
        dispatch_async(dispatch_get_main_queue(), ^{
            if (!HasDeferredMessage())
                return;
            
            ShowAlert(GetDeferredMessage());
            ResetDeferredMessage();
        });
    }
    else
    {
        ShowAlert(msg);
    }

    AppBase::Alert(msg, deferred);
}

void AppMac::SetWindowTitle(const std::string& title_text)
{
    META_FUNCTION_TASK();
    META_UNUSED(title_text);
}

void AppMac::SetWindow(UIWindow* ns_window)
{
    META_FUNCTION_TASK();
    m_ns_window = ns_window;
}

void AppMac::ShowAlert(const Message& msg)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_ns_app_delegate);

    [m_ns_app_delegate alert: MacOS::ConvertToNsType<std::string, NSString*>(msg.title)
             withInformation: MacOS::ConvertToNsType<std::string, NSString*>(msg.information)
                    andStyle: ConvertMessageTypeToNsAlertStyle(msg.type)];

    AppBase::ShowAlert(msg);
}

bool AppMac::SetFullScreen(bool is_full_screen)
{
    META_FUNCTION_TASK();
    if (!AppBase::SetFullScreen(is_full_screen))
        return false;

    return true;
}

void AppMac::Close()
{
    META_FUNCTION_TASK();
}

} // namespace Methane::Platform