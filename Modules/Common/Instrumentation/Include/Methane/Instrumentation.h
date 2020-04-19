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

NOTE:
    This header is force included first in every source file,
    which is linked with cmake interface target MethaneInstrumentation,
    when METHANE_TRACY_PROFILING_ENABLED = ON

******************************************************************************/

#pragma once

// Include Tracy header implementation first
// before memory allocation operator overrides
// to minimize possibility of Tracy code self-instrumentation
#include <Tracy.hpp>
#include <TracyC.h>

#include "IttApiHelper.h"
#include "ScopeTimer.h"

#ifdef TRACY_ENABLE

// Override memory allocation operators new/delete with Tracy instrumentation
#include "InstrumentMemoryAllocations.h"

#endif // TRACY_ENABLE

ITT_DOMAIN_EXTERN();

#define META_CPU_FRAME_DELIMITER() \
    FrameMark \
    ITT_THREAD_MARKER("Methane-Frame-Delimiter")

#define META_CPU_FRAME_START(/*const char* */name) \
    TracyCFrameMarkStart(name)

#define META_CPU_FRAME_END(/*const char* */name) \
    TracyCFrameMarkEnd(name)

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

#define META_CHART_CONFIG(/*const char* */name, /*tracy::PlotFormatType */tracy_format) \
    TracyPlotConfig(name, tracy_format);

#define META_CHART_VALUE(/*const char* */name, /*int64_t | float | double */value) \
    TracyPlot(name, value)

#ifdef METHANE_LOGGING_ENABLED

#include <Methane/Platform/Utils.h>

#define META_LOG(/*const std::string& */message) \
    Methane::Platform::PrintToDebugOutput(message)

#else // METHANE_LOGGING_ENABLED

#define META_LOG(/*const std::string& */message)

#endif // METHANE_LOGGING_ENABLED