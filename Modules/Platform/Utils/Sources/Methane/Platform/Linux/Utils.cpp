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

FILE: Methane/Platform/MacOS/Utils.mm
MacOS platform utility functions.

******************************************************************************/

#include <Methane/Platform/MacOS/Utils.hh>
#include <Methane/Instrumentation.h>

#include <string_view>
#include <iostream>

#include <libgen.h>
#include <unistd.h>
#include <linux/limits.h>

namespace Methane::Platform
{

static const std::string_view g_proc_self_exe = "/proc/self/exe";

void PrintToDebugOutput(__attribute__((unused)) std::string_view msg)
{
    META_FUNCTION_TASK();
    std::cerr << msg;
    TracyMessage(msg.data(), msg.size());
}

std::string GetExecutableDir()
{
    META_FUNCTION_TASK();
    char exe_path[PATH_MAX] = { 0 }; // NOSONAR
    if (readlink(g_proc_self_exe.data(), exe_path, PATH_MAX) <= 0)
        return std::string();

    return dirname(exe_path);
}

std::string GetExecutableFileName()
{
    META_FUNCTION_TASK();
    char exe_path[PATH_MAX] = { 0 }; // NOSONAR
    if (readlink(g_proc_self_exe.data(), exe_path, PATH_MAX) <= 0)
        return std::string();

    return basename(exe_path);
}

std::string GetResourceDir()
{
    return GetExecutableDir();
}

} // namespace Methane::Platform
