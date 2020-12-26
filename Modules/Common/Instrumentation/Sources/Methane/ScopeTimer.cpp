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

FILE: Methane/ScopeTimer.cpp
Code scope measurement timer with aggregating and averaging of timings.

******************************************************************************/

#include <Methane/ScopeTimer.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <sstream>
#include <chrono>

namespace Methane
{

ScopeTimer::Aggregator& ScopeTimer::Aggregator::Get()
{
    META_FUNCTION_TASK();
    static Aggregator s_scope_aggregator;
    return s_scope_aggregator;
}

ScopeTimer::Aggregator::~Aggregator()
{
    META_FUNCTION_TASK();
    Flush();
}

void ScopeTimer::Aggregator::Flush()
{
    META_FUNCTION_TASK();
    if (m_logger_ptr)
    {
        LogTimings(*m_logger_ptr);
    }

    m_timing_by_scope_id.clear();
    m_scope_id_by_name.clear();
    m_new_scope_id = 0U;
}

void ScopeTimer::Aggregator::LogTimings(ILogger& logger)
{
    META_FUNCTION_TASK();
    if (m_timing_by_scope_id.empty())
        return;

    std::stringstream ss;
    ss << std::endl << "Aggregated performance timings:" << std::endl;

    for (const auto& [scope_name, scope_id] : m_scope_id_by_name)
    {
        META_CHECK_ARG_LESS(scope_id, m_timing_by_scope_id.size());

        const Timing& scope_timing = m_timing_by_scope_id[scope_id];
        const double total_duration_sec = std::chrono::duration_cast<std::chrono::duration<double>>(scope_timing.duration).count();
        const double average_duration_ms = total_duration_sec * 1000.0 / scope_timing.count;

        ss << "  - "       << scope_name
           << ": "         << std::fixed << average_duration_ms
           << " ms. with " << scope_timing.count
           << " invocations count;" << std::endl;
    }

    logger.Log(ss.str());
}

ScopeTimer::Registration ScopeTimer::Aggregator::RegisterScope(const char* scope_name)
{
    META_FUNCTION_TASK();
    const auto [ scope_name_and_id_it, scope_added ] = m_scope_id_by_name.try_emplace(scope_name, m_new_scope_id);
    if (scope_added)
    {
        m_new_scope_id++;
        m_timing_by_scope_id.resize(m_new_scope_id);
        m_counters_by_scope_id.emplace_back(ITT_COUNTER_INIT(scope_name_and_id_it->first, g_methane_itt_domain_name));
        TracyPlotConfig(scope_name_and_id_it->first, tracy::PlotFormatType::Number);
    }
    return Registration{ scope_name_and_id_it->first, scope_name_and_id_it->second };
}

void ScopeTimer::Aggregator::AddScopeTiming(const Registration& scope_registration, TimeDuration duration)
{
    META_FUNCTION_TASK();
    ITT_COUNTER_VALUE(m_counters_by_scope_id[scope_registration.id], std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count());
    TracyPlot(scope_registration.name, std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count());

    META_CHECK_ARG_LESS(scope_registration.id, m_timing_by_scope_id.size());
    Timing& scope_timing = m_timing_by_scope_id[scope_registration.id];
    scope_timing.count++;
    scope_timing.duration += duration;
}

ScopeTimer::ScopeTimer(const char* scope_name)
    : Timer()
    , m_registration(Aggregator::Get().RegisterScope(scope_name))
{
    META_FUNCTION_TASK();
}

ScopeTimer::~ScopeTimer()
{
    META_FUNCTION_TASK();
    Aggregator::Get().AddScopeTiming(m_registration, GetElapsedDuration());
}

} // namespace Methane
