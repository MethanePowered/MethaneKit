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

FILE: Methane/Platform/Windows/Utils.cpp
Windows platform utility functions.

******************************************************************************/

#include <Methane/Platform/Windows/Utils.h>
#include <Methane/Instrumentation.h>

#include <Windows.h>
#include <shellapi.h>

#include <wrl.h>
#include <nowide/convert.hpp>

namespace wrl = Microsoft::WRL;

namespace Methane::Platform
{

void PrintToDebugOutput(const std::string& msg)
{
    ITT_FUNCTION_TASK();
    OutputDebugStringA((msg + "\n").c_str());
}

std::string GetExecutableDir()
{
    ITT_FUNCTION_TASK();

    WCHAR path[512];
    DWORD size = GetModuleFileName(nullptr, path, _countof(path));

    if (size == 0 || size == _countof(path))
    {
        return "";
    }

    WCHAR* last_slash = wcsrchr(path, L'\\');
    if (last_slash)
    {
        *(last_slash + 1) = NULL;
    }

    return nowide::narrow(path);
}

std::string GetExecutableFileName()
{
    ITT_FUNCTION_TASK();

    WCHAR path[512];
    DWORD size = GetModuleFileName(nullptr, path, _countof(path));

    if (size == 0 || size == _countof(path))
    {
        return "";
    }

    WCHAR* last_slash = wcsrchr(path, L'\\');
    return last_slash ? nowide::narrow(last_slash + 1) : "";
}

std::string GetResourceDir()
{
    ITT_FUNCTION_TASK();
    return GetExecutableDir();
}

namespace Windows
{

void GetDesktopResolution(uint32_t& width, uint32_t& height)
{
    ITT_FUNCTION_TASK();
    RECT       desktop;
    const HWND h_desktop = GetDesktopWindow();
    GetWindowRect(h_desktop, &desktop);
    width = static_cast<uint32_t>(desktop.right);
    height = static_cast<uint32_t>(desktop.bottom);
}

} // namespace Windows
} // namespace Methane::Platform
