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

FILE: Methane/Platform/Utils.h
Methane platform utility functions

******************************************************************************/

#pragma once

#if defined _WIN32

#include "Windows/Utils.h"

#elif defined __APPLE__

#include "Apple/Utils.hh"

#elif defined __linux__

#include "Linux/Utils.h"

#endif

#include <string>
#include <string_view>
#include <vector>

namespace Methane::Platform
{

void PrintToDebugOutput(std::string_view msg);
std::string GetExecutableDir();
std::string GetExecutableFileName();
std::string GetResourceDir();
std::vector<std::string_view> SplitString(const std::string_view str, const char delimiter,
                                          bool with_empty_parts = false, size_t max_chunk_size = std::numeric_limits<size_t>::max());

} // namespace Methane::Platform
