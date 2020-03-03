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

FILE: Methane/ScopeTimer.h
Code scope measurement timer with aggregating and averaging of timings.

******************************************************************************/

#pragma once

#include "ILogger.h"

#include <Methane/Timer.hpp>
#include <Methane/Memory.hpp>

#include <string>
#include <map>

namespace Methane
{

class ScopeTimer : protected Timer
{
public:
    class Aggregator
    {
    public:
        struct Timing
        {
            TimeDuration duration;
            uint32_t     count    = 0u;
        };

        static Aggregator& Get();
        ~Aggregator();

        void SetLogger(Ptr<ILogger> sp_logger)   { m_sp_logger = std::move(sp_logger); }
        const Ptr<ILogger>& GetLogger() const { return m_sp_logger; }

        void AddScopeTiming(const std::string& scope_name, TimeDuration duration);
        const Timing& GetScopeTiming(const std::string& scope_name) const;

        void LogTimings(ILogger& logger);
        void Flush();

    private:
        Aggregator() = default;

        using ScopeTimings = std::map<std::string, Timing>;

        ScopeTimings m_timing_by_scope_name;
        Ptr<ILogger> m_sp_logger;
    };

    template<typename TLogger>
    static void InitializeLogger()
    {
        Aggregator::Get().SetLogger(std::make_shared<TLogger>());
    }

    ScopeTimer(std::string scope_name);
    ~ScopeTimer();

    const std::string& GetScopeName() const { return m_scope_name; }

private:
    std::string m_scope_name;
};

} // namespace Methane

#ifdef SCOPE_TIMERS_ENABLED

#define SCOPE_TIMER_INITIALIZE(LOGGER_TYPE) Methane::ScopeTimer::InitializeLogger<LOGGER_TYPE>()
#define SCOPE_TIMER(SCOPE_NAME) Methane::ScopeTimer scope_timer(SCOPE_NAME)
#define FUNCTION_SCOPE_TIMER() SCOPE_TIMER(__func__)
#define FLUSH_SCOPE_TIMINGS() Methane::ScopeTimer::Aggregator::Get().Flush()

#else // ifdef SCOPE_TIMERS_ENABLED

#define SCOPE_TIMER_INITIALIZE(LOGGER_TYPE)
#define SCOPE_TIMER(SCOPE_NAME)
#define FUNCTION_SCOPE_TIMER()
#define FLUSH_SCOPE_TIMINGS()

#endif // ifdef SCOPE_TIMERS_ENABLED
