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

FILE: Methane/Instrumentation.h
Common header for instrumentation of the Methane Kit modules with ITT macroses,
Defines common ITT domain required for instrumentation.

******************************************************************************/

#pragma once

#include "IttApiHelper.h"
#include "ScopeTimer.h"

#include <Tracy.hpp>

ITT_DOMAIN_EXTERN();

#define META_CPU_FRAME_DELIMITER() \
    FrameMark \
    ITT_THREAD_MARKER("Frame-Delimiter");

#define META_SCOPE_TASK(/*const char* */name) \
    ZoneScopedN(name) \
    ITT_SCOPE_TASK(name)

#define META_FUNCTION_TASK() \
    ZoneScoped \
    ITT_FUNCTION_TASK()

#define META_GLOBAL_MARKER(/*const char* */name) \
    ITT_GLOBAL_MARKER(name)
#define META_PROCESS_MARKER(/*const char* */name) \
    ITT_PROCESS_MARKER(name)
#define META_THREAD_MARKER(/*const char* */name) \
    ITT_THREAD_MARKER(name)
#define META_TASK_MARKER(/*const char* */name) \
    ITT_TASK_MARKER(name)

#define META_FUNCTION_GLOBAL_MARKER() \
    ITT_FUNCTION_GLOBAL_MARKER()
#define META_FUNCTION_PROCESS_MARKER() \
    ITT_FUNCTION_PROCESS_MARKER()
#define META_FUNCTION_THREAD_MARKER() \
    ITT_FUNCTION_THREAD_MARKER()
#define META_FUNCTION_TASK_MARKER() \
    ITT_FUNCTION_TASK_MARKER()
