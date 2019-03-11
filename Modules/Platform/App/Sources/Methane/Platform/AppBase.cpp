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

FILE: Methane/Platform/AppBase.cpp
Base application interface and platform-independent implementation.

******************************************************************************/

#include <Methane/Platform/AppBase.h>
#include <Methane/Platform/Utils.h>

#include <cassert>

using namespace Methane;
using namespace Methane::Platform;

AppBase::AppBase(const AppBase::Settings& settings)
    : m_settings(settings)
{
}

int AppBase::Run(const RunArgs& args)
{
    const std::vector<std::string> cmd_args = Methane::Platform::GetCommandLineArgs(args.cmd_arg_count, args.cmd_arg_values);
    ParseCommandLine(cmd_args);
    return 0;
}

void AppBase::Init()
{
    m_initialized = true;
}

void AppBase::ChangeWindowBounds(const Data::FrameRect& window_bounds)
{
    m_window_bounds = window_bounds;
}

void AppBase::Alert(const Message& msg, bool deferred)
{
    if (!deferred)
        return;

    m_sp_deferred_message.reset(new Message(msg));
}

bool AppBase::HasError() const
{
    return m_sp_deferred_message ? m_sp_deferred_message->type == Message::Type::Error : false;
}
