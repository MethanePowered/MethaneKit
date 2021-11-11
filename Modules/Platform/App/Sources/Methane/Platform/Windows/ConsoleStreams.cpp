/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy

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

FILE: Methane/Platform/Windows/ConsoleStreams.cpp
IO Stream class used for standard and error output redirection from GUI app to console.

******************************************************************************/

#include "ConsoleStreams.h"

#include <Methane/Instrumentation.h>

#include <io.h>

namespace Methane::Platform
{

static bool IsMappedToFile(FILE* std_stream)
{
    META_FUNCTION_TASK();
    const int file_desc = _fileno(std_stream);
    const HANDLE file_handle = reinterpret_cast<HANDLE>(_get_osfhandle(file_desc));
    return file_handle != INVALID_HANDLE_VALUE;
}

IOStream::IOStream(FILE* std_stream, DWORD std_handle)
    : m_std_stream(std_stream)
    , m_std_handle(std_handle)
{
    META_FUNCTION_TASK();
}

IOStream::~IOStream()
{
    META_FUNCTION_TASK();
    if (m_is_redirected && m_std_stream)
        fclose(m_std_stream);
}

bool IOStream::RedirectToFile(std::string_view file_name, std::string_view file_mode)
{
    META_FUNCTION_TASK();
    if (m_is_redirected || GetStdHandle(m_std_handle) == INVALID_HANDLE_VALUE)
        return false;

    if (IsMappedToFile(m_std_stream))
        return true;

    FILE* target_stream = nullptr;
    const errno_t error = freopen_s(&target_stream, file_name.data(), file_mode.data(), m_std_stream);
    if (error)
        return false;

    setvbuf(m_std_stream, nullptr, _IONBF, 0);
    m_std_stream = target_stream;
    return true;
    
}

ConsoleStreams::ConsoleStreams()
    : m_output_stream(stdout, STD_OUTPUT_HANDLE)
    , m_error_stream(stderr, STD_ERROR_HANDLE)
{
    META_FUNCTION_TASK();
}

bool ConsoleStreams::Attach()
{
    META_FUNCTION_TASK();
    return AttachConsole(ATTACH_PARENT_PROCESS) &&
           m_output_stream.RedirectToFile("CONOUT$", "w") &&
           m_error_stream.RedirectToFile("CONOUT$", "w");
}

} // namespace Methane::Platform