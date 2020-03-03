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

#import <AppKit/AppKit.h>

#include <cassert>

namespace Methane::Platform
{

void PrintToDebugOutput(const std::string& msg)
{
    ITT_FUNCTION_TASK();
    NSLog(@"%s", msg.c_str());
}

std::string GetExecutableDir()
{
    ITT_FUNCTION_TASK();
    std::string exe_dir;
    @autoreleasepool
    {
        exe_dir = [[[NSBundle mainBundle] executablePath] UTF8String];
        auto dir_separator_pos = exe_dir.rfind("/");
        if (dir_separator_pos != std::string::npos)
        {
            exe_dir.erase(dir_separator_pos + 1);
        }
    }
    return exe_dir;
}

std::string GetExecutableFileName()
{
    ITT_FUNCTION_TASK();
    std::string exe_file;
    @autoreleasepool
    {
        exe_file = [[[NSBundle mainBundle] executablePath] UTF8String];
        auto dir_separator_pos = exe_file.rfind("/");
        if (dir_separator_pos != std::string::npos)
        {
            exe_file.erase(0, dir_separator_pos + 1);
        }
    }
    return exe_file;
}

std::string GetResourceDir()
{
    ITT_FUNCTION_TASK();
    std::string res_dir;
    @autoreleasepool
    {
        res_dir = [[[NSBundle mainBundle] resourcePath] UTF8String];
        res_dir += "/";
    }
    return res_dir;
}

} // namespace Methane::Platform
