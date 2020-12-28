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

FILE: Methane/Linux/Instrumentation.cpp
Linux implementation of the platform specific instrumentation functions.

******************************************************************************/

#include <pthread.h>

#include <cstring>

namespace Methane
{

void SetThreadName(const char* name)
{
    if(strlen(name) < 16)
    {
        pthread_setname_np(pthread_self(), name);
        return;
    }

    std::array<char, 16> buf;
    memcpy(buf, name, 15);
    buf[15] = '\0';
    pthread_setname_np(pthread_self(), buf);
}

} // namespace Methane