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

FILE: Methane/Platform/Windows/Utils.cpp
Windows platform utility functions.

******************************************************************************/

#include <Methane/Platform/Windows/Utils.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <Windows.h>
#include <shellapi.h>

#include <nowide/convert.hpp>

namespace Methane::Platform
{

void PrintToDebugOutput(const std::string& msg)
{
    META_FUNCTION_TASK();
    OutputDebugStringA(fmt::format("{}\n", msg).c_str());
    TracyMessage(msg.c_str(), msg.size());
}

inline std::wstring GetExecutableFilePath()
{
    std::array<wchar_t, 512> path_chars{ };
    const DWORD path_size = GetModuleFileNameW(nullptr, path_chars.data(), static_cast<DWORD>(path_chars.size()));
    META_CHECK_ARG_RANGE_DESCR(path_size, 1, path_chars.size(), "failed to get module file path");
    return std::wstring(path_chars.data(), path_size);
}

std::string GetExecutableDir()
{
    META_FUNCTION_TASK();
    std::wstring path = GetExecutableFilePath();

    const size_t last_slash_pos = path.rfind(L'\\');
    META_CHECK_ARG_NOT_EQUAL_DESCR(last_slash_pos, std::wstring::npos, "module file path does not contain directory separator");

    return nowide::narrow(path.substr(0, last_slash_pos));
}

std::string GetExecutableFileName()
{
    META_FUNCTION_TASK();
    std::wstring path = GetExecutableFilePath();

    const size_t last_slash_pos = path.rfind(L'\\');
    META_CHECK_ARG_NOT_EQUAL_DESCR(last_slash_pos, std::wstring::npos, "module file path does not contain directory separator");

    return nowide::narrow(path.substr(last_slash_pos + 1));
}

std::string GetResourceDir()
{
    META_FUNCTION_TASK();
    return GetExecutableDir();
}

namespace Windows
{

void GetDesktopResolution(uint32_t& width, uint32_t& height) noexcept
{
    META_FUNCTION_TASK();
    const HWND h_desktop = GetDesktopWindow();

    RECT desktop;
    GetWindowRect(h_desktop, &desktop);

    width = static_cast<uint32_t>(desktop.right);
    height = static_cast<uint32_t>(desktop.bottom);
}

bool IsDeveloperModeEnabled() noexcept
{
    HKEY h_key{};
    auto err = RegOpenKeyExW(HKEY_LOCAL_MACHINE, LR"(SOFTWARE\Microsoft\Windows\CurrentVersion\AppModelUnlock)", 0, KEY_READ, &h_key);
    if (err != ERROR_SUCCESS)
        return false;

    DWORD value{};
    DWORD dword_size = sizeof(DWORD);
    err = RegQueryValueExW(h_key, L"AllowDevelopmentWithoutDevLicense", nullptr, nullptr, reinterpret_cast<LPBYTE>(&value), &dword_size);
    RegCloseKey(h_key);
    if (err != ERROR_SUCCESS)
        return false;

    return value != 0;
}

} // namespace Windows
} // namespace Methane::Platform
