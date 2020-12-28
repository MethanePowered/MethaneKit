/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/Windows/Instrumentation.cpp
Windows implementation of the platform specific instrumentation functions.

******************************************************************************/

#include <Windows.h>

#include <nowide/convert.hpp>

namespace Methane
{

#if defined _MSC_VER

#pragma pack(push, 8)
struct ThreadNameInfo
{
    DWORD  dw_type;
    LPCSTR sz_name;
    DWORD  dw_thread_id;
    DWORD  dw_flags;
};
#pragma pack(pop)

static void SetLegacyThreadName(const char* name)
{
    // Set thread name with legacy exception way
    constexpr DWORD msvc_exception = 0x406D1388;
    ThreadNameInfo info {
        0x1000,
        name,
        GetCurrentThreadId(),
        0
    };

    __try
    {
        RaiseException(msvc_exception, 0, sizeof(info) / sizeof(ULONG_PTR), reinterpret_cast<ULONG_PTR*>(&info));
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        // Exception is not handled intentionally, it is used to set Windows thread name
    }
}

#else

static void SetLegacyThreadName(const char*) { }

#endif

extern "C" typedef HRESULT (WINAPI* SetThreadDescriptionFn)(HANDLE, PCWSTR);

void SetThreadName(const char* name)
{
    if (static auto s_set_thread_description_fn = reinterpret_cast<SetThreadDescriptionFn>(GetProcAddress(GetModuleHandleA("kernel32.dll"), "SetThreadDescription"));
        s_set_thread_description_fn)
    {
        const std::wstring w_name = nowide::widen(name);
        s_set_thread_description_fn(GetCurrentThread(), w_name.c_str());
    }
    else
    {
        SetLegacyThreadName(name);
    }
}

} // namespace Methane