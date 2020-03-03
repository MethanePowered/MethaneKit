/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/ScopeTimer.cpp
Code scope measurement timer with aggregating and averaging of timings.

******************************************************************************/

#include <Methane/ScopeTimer.h>

#include <Methane/Instrumentation.h>

#include <sstream>
#include <chrono>

namespace Methane
{

ScopeTimer::Aggregator& ScopeTimer::Aggregator::Get()
{
    ITT_FUNCTION_TASK();
    static Aggregator s_scope_aggregator;
    return s_scope_aggregator;
}

ScopeTimer::Aggregator::~Aggregator()
{
    ITT_FUNCTION_TASK();
    Flush();
}

void ScopeTimer::Aggregator::Flush()
{
    ITT_FUNCTION_TASK();
    if (m_sp_logger)
    {
        LogTimings(*m_sp_logger);
    }
    m_timing_by_scope_name.clear();
}

void ScopeTimer::Aggregator::LogTimings(ILogger& logger)
{
    ITT_FUNCTION_TASK();
    if (m_timing_by_scope_name.empty())
        return;

    std::stringstream ss;
    ss << std::endl << "Aggregated performance timings:" << std::endl;

    for (const auto& scope_name_and_timing : m_timing_by_scope_name)
    {
        const Timing& scope_timing = scope_name_and_timing.second;
        const double total_duration_sec = std::chrono::duration_cast<std::chrono::duration<double>>(scope_timing.duration).count();
        const double average_duration_ms = total_duration_sec * 1000.0 / scope_timing.count;

        ss << "  - " << scope_name_and_timing.first << ": "
           << std::fixed << average_duration_ms << " ms. with " << scope_timing.count << " invocations count;" << std::endl;
    }

    logger.Log(ss.str());
}

void ScopeTimer::Aggregator::AddScopeTiming(const std::string& scope_name, TimeDuration duration)
{
    ITT_FUNCTION_TASK();

    Timing& scope_timing = m_timing_by_scope_name[scope_name];
    scope_timing.count++;
    scope_timing.duration += duration;
}

const ScopeTimer::Aggregator::Timing& ScopeTimer::Aggregator::GetScopeTiming(const std::string& scope_name) const
{
    ITT_FUNCTION_TASK();

    static Timing s_empty_timing = {};
    auto   scope_timing_it = m_timing_by_scope_name.find(scope_name);
    return scope_timing_it != m_timing_by_scope_name.end() ? scope_timing_it->second : s_empty_timing;
}

ScopeTimer::ScopeTimer(std::string scope_name)
    : Timer()
    , m_scope_name(std::move(scope_name))
{
    ITT_FUNCTION_TASK();
}

ScopeTimer::~ScopeTimer()
{
    ITT_FUNCTION_TASK();
    Aggregator::Get().AddScopeTiming(m_scope_name, GetElapsedDuration());
}

} // namespace Methane
