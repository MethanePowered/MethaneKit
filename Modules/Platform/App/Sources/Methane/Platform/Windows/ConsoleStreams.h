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

FILE: Methane/Platform/Windows/ConsoleStreams.h
IOStream class is used for standard and error output redirection
 from GUI Windows app to console output when it was started from terminal.

******************************************************************************/
#pragma once

#include <Windows.h>

#include <stdio.h>
#include <string_view>

namespace Methane::Platform
{

class IOStream // NOSONAR - destructor is required in this class
{
public:
    IOStream(FILE* std_stream, DWORD std_handle);
    ~IOStream();

    IOStream(const IOStream& other) = delete;
    IOStream(IOStream&& other) noexcept = delete;

    IOStream& operator=(const IOStream& other) = delete;
    IOStream& operator=(IOStream&& other) noexcept = delete;

    bool RedirectToFile(std::string_view file_name, std::string_view file_mode);

private:
    FILE* m_std_stream;
    DWORD m_std_handle;
    bool  m_is_redirected = false;
};

class ConsoleStreams
{
public:
    ConsoleStreams() = default;

    const IOStream& GetOutputStream() const noexcept { return m_output_stream; }
    const IOStream& GetErrorStream() const noexcept  { return m_error_stream; }

    bool Attach();

private:
    IOStream m_output_stream = { stdout, STD_OUTPUT_HANDLE };
    IOStream m_error_stream  = { stderr, STD_ERROR_HANDLE  };
};

} // namespace Methane::Platform