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

FILE: Methane/Platform/MacOS/Utils.mm
MacOS platform utility functions.

******************************************************************************/

#include <Methane/Platform/MacOS/Utils.hh>
#include <Methane/Instrumentation.h>

#include <cassert>
#include <stdexcept>

namespace Methane::Platform
{

void PrintToDebugOutput(const std::string& msg)
{
    ITT_FUNCTION_TASK();
    throw std::runtime_error("Method is not implemented yet.");
}

std::string GetExecutableDir()
{
    ITT_FUNCTION_TASK();
    throw std::runtime_error("Method is not implemented yet.");
}

std::string GetExecutableFileName()
{
    ITT_FUNCTION_TASK();
    throw std::runtime_error("Method is not implemented yet.");
}

std::string GetResourceDir()
{
    ITT_FUNCTION_TASK();
    throw std::runtime_error("Method is not implemented yet.");
}

} // namespace Methane::Platform
