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

FILE: Methane/Platform/Linux/AppLin.cpp
Linux application implementation.

******************************************************************************/

#include <Methane/Platform/Linux/AppLin.h>
#include <Methane/Platform/Utils.h>
#include <Methane/Instrumentation.h>

namespace Methane::Platform
{

AppLin::AppLin(const AppBase::Settings& settings)
    : AppBase(settings)
{
    ITT_FUNCTION_TASK();
}

int AppLin::Run(const RunArgs& args)
{
    return 0;
}

void AppLin::Alert(const Message& msg, bool deferred)
{
    ITT_FUNCTION_TASK();
    AppBase::Alert(msg, deferred);
}

void AppLin::ShowAlert(const Message& msg)
{
    ITT_FUNCTION_TASK();
}

void AppLin::SetWindowTitle(const std::string& title_text)
{
    ITT_FUNCTION_TASK();
}

bool AppLin::SetFullScreen(bool is_full_screen)
{
    ITT_FUNCTION_TASK();
    if (!AppBase::SetFullScreen(is_full_screen))
        return false;

    return true;
}

void AppLin::Close()
{
    ITT_FUNCTION_TASK();
}

} // namespace Methane::Platform