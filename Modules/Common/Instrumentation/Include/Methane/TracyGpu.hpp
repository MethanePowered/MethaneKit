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

FILE: Methane/TracyGpu.hpp
Tracy GPU instrumentation helpers

******************************************************************************/

#pragma once

#ifdef TRACY_GPU_ENABLE
#include "IttApiHelper.h"
#include <Tracy.hpp>
#include <client/TracyProfiler.hpp>
#include <client/TracyCallstack.hpp>
#endif

#include <assert.h>
#include <stdlib.h>
#include <mutex>

namespace Methane::Tracy
{

class GpuScope;

using QueryId   = uint16_t;
using Timestamp = int64_t;
using ThreadId  = uint64_t;

class GpuContext
{
    friend class GpuScope;

public:
    struct Settings
    {
        Timestamp gpu_timestamp = 0;
        Timestamp cpu_timestamp = 0;
        float   gpu_time_period = 1.f; // number of nanoseconds required for a timestamp query to be incremented by 1
        bool    is_thread_local = false;

#ifdef TRACY_GPU_ENABLE

        Settings()
            : gpu_timestamp(tracy::Profiler::GetTime())
            , cpu_timestamp(gpu_timestamp)
        { }

        explicit Settings(Timestamp gpu_timestamp, float gpu_time_period = 1.f, bool is_thread_local = false)
            : gpu_timestamp(gpu_timestamp)
            , cpu_timestamp(tracy::Profiler::GetTime())
            , gpu_time_period(gpu_time_period)
            , is_thread_local(is_thread_local)
        { }

#else // TRACY_GPU_ENABLE

        Settings() = default;
        explicit Settings(Timestamp) { }
        Settings(Timestamp, float) { }
        Settings(Timestamp, float, bool) { }

#endif // TRACY_GPU_ENABLE
    };

#ifdef TRACY_GPU_ENABLE
    explicit GpuContext(const Settings& settings)
        : m_id(tracy::GetGpuCtxCounter().fetch_add( 1, std::memory_order_relaxed ))
    {
        ITT_FUNCTION_TASK();

        assert(m_id != 255);
        auto item = tracy::Profiler::QueueSerial();
        tracy::MemWrite(&item->hdr.type, tracy::QueueType::GpuNewContext);
        tracy::MemWrite(&item->gpuNewContext.cpuTime, settings.cpu_timestamp);
        tracy::MemWrite(&item->gpuNewContext.gpuTime, settings.gpu_timestamp);
        tracy::MemWrite(&item->gpuNewContext.period,  settings.gpu_time_period);
        tracy::MemWrite(&item->gpuNewContext.context, m_id);
        tracy::MemWrite(&item->gpuNewContext.accuracyBits, uint8_t(0));
        if (settings.is_thread_local)
        {
            const auto thread = tracy::GetThreadHandle();
            tracy::MemWrite(&item->gpuNewContext.thread, thread);
        }
        else
        {
            memset( &item->gpuNewContext.thread, 0, sizeof( item->gpuNewContext.thread ) );
        }

#ifdef TRACY_ON_DEMAND
        tracy::GetProfiler().DeferItem(*item);
#endif
        tracy:: Profiler::QueueSerialFinish();
    }

private:
    tracy_force_inline QueryId NextQueryId()
    {
        std::lock_guard<std::mutex> lock_guard(m_query_mutex);
        m_query_id = (m_query_id + 1) % m_query_count;
        return m_query_id;
    }

    tracy_force_inline uint8_t GetId() const
    {
        return m_id;
    }

    const uint8_t m_id;
    const QueryId m_query_count = std::numeric_limits<QueryId>::max();
    QueryId       m_query_id    = 0u;
    std::mutex    m_query_mutex;

#else // TRACY_GPU_ENABLE

    explicit GpuContext(const Settings&) { }

#endif // TRACY_GPU_ENABLE
};

class GpuScope
{
public:
#ifdef TRACY_GPU_ENABLE
    enum class State
    {
        Begun,
        Ended,
        Completed
    };

    explicit GpuScope(GpuContext& context)
        : m_context(context)
    {
        ITT_FUNCTION_TASK();
    }

    tracy_force_inline void Begin(const tracy::SourceLocationData* src_location, int call_stack_depth = 0)
    {
        ITT_FUNCTION_TASK();

#ifdef TRACY_ON_DEMAND
        m_is_active = tracy::GetProfiler().IsConnected();
#endif
        if (!m_is_active)
            return;

        if (m_state != State::Completed)
            throw std::logic_error("GPU scope can be begun only from completed state.");

        m_state           = State::Begun;
        m_begin_thread_id = tracy::GetThreadHandle();
        m_begin_query_id  = m_context.NextQueryId();

        auto item = tracy::Profiler::QueueSerial();
        const tracy::QueueType item_type = call_stack_depth > 0 ? tracy::QueueType::GpuZoneBeginCallstackSerial : tracy::QueueType::GpuZoneBeginSerial;
        tracy::MemWrite(&item->hdr.type,             item_type);
        tracy::MemWrite(&item->gpuZoneBegin.cpuTime, tracy::Profiler::GetTime());
        tracy::MemWrite(&item->gpuZoneBegin.srcloc,  reinterpret_cast<uint64_t>(src_location));
        tracy::MemWrite(&item->gpuZoneBegin.thread,  m_begin_thread_id);
        tracy::MemWrite(&item->gpuZoneBegin.queryId, m_begin_query_id);
        tracy::MemWrite(&item->gpuZoneBegin.context, m_context.GetId());
        tracy::Profiler::QueueSerialFinish();

        if (call_stack_depth)
        {
            tracy::GetProfiler().SendCallstack(call_stack_depth);
        }
    }

    tracy_force_inline void End()
    {
        ITT_FUNCTION_TASK();
        if (!m_is_active)
            return;

        if (m_state != State::Begun)
            throw std::logic_error("GPU scope can be ended only from begun state.");

        m_state        = State::Ended;
        m_end_query_id = m_context.NextQueryId();

        auto item = tracy::Profiler::QueueSerial();
        tracy::MemWrite(&item->hdr.type,           tracy::QueueType::GpuZoneEndSerial);
        tracy::MemWrite(&item->gpuZoneEnd.cpuTime, tracy::Profiler::GetTime());
        tracy::MemWrite(&item->gpuZoneEnd.thread,  m_begin_thread_id);
        tracy::MemWrite(&item->gpuZoneEnd.queryId, m_end_query_id);
        tracy::MemWrite(&item->gpuZoneEnd.context, m_context.GetId());
        tracy::Profiler::QueueSerialFinish();
    }

    tracy_force_inline void Complete(Timestamp gpu_begin_timestamp, Timestamp gpu_end_timestamp)
    {
        ITT_FUNCTION_TASK();
        if (!m_is_active)
            return;

        if (m_state != State::Ended)
            throw std::logic_error("GPU scope can be completed only from ended state.");

        m_state = State::Completed;

        if (gpu_begin_timestamp < 0 || gpu_begin_timestamp > gpu_end_timestamp)
            throw std::logic_error("GPU begin timestamp should be less or equal to end timestamp and both should be positive.");

        auto begin_item = tracy::Profiler::QueueSerial();
        tracy::MemWrite(&begin_item->hdr.type, tracy::QueueType::GpuTime);
        tracy::MemWrite(&begin_item->gpuTime.gpuTime, gpu_begin_timestamp);
        tracy::MemWrite(&begin_item->gpuTime.queryId, m_begin_query_id);
        tracy::MemWrite(&begin_item->gpuTime.context, m_context.GetId());
        tracy::Profiler::QueueSerialFinish();

        auto end_item = tracy::Profiler::QueueSerial();
        tracy::MemWrite(&end_item->hdr.type, tracy::QueueType::GpuTime);
        tracy::MemWrite(&end_item->gpuTime.gpuTime, gpu_end_timestamp);
        tracy::MemWrite(&end_item->gpuTime.queryId, m_end_query_id);
        tracy::MemWrite(&end_item->gpuTime.context, m_context.GetId());
        tracy::Profiler::QueueSerialFinish();
    }

    tracy_force_inline State GetState() const { return m_state; }

private:
    GpuContext& m_context;
    State       m_state           = State::Completed;
    ThreadId    m_begin_thread_id = 0u;
    QueryId     m_begin_query_id  = 0u;
    QueryId     m_end_query_id    = 0u;
    bool        m_is_active       = true;
};

} // namespace Methane::Tracy

#define TRACE_SOURCE_LOCATION_TYPE tracy::SourceLocationData
#define CREATE_TRACY_SOURCE_LOCATION(name) \
    new TRACE_SOURCE_LOCATION_TYPE{ name, __FUNCTION__,  __FILE__, static_cast<uint32_t>(__LINE__), 0 }

#define TRACY_GPU_SCOPE_TYPE GpuScope
#define TRACY_GPU_SCOPE_INIT(gpu_context) gpu_context

#define TRACY_GPU_SCOPE_BEGIN_AT_LOCATION(gpu_scope, p_location) \
    gpu_scope.Begin(p_location)

#define TRACY_GPU_SCOPE_TRY_BEGIN_AT_LOCATION(gpu_scope, p_location) \
    if (gpu_scope.GetState() != Methane::Tracy::GpuScope::State::Begun) \
        gpu_scope.Begin(p_location)

#define TRACY_GPU_SCOPE_BEGIN(gpu_scope, name) \
    static const tracy::SourceLocationData TracyConcat(__tracy_gpu_source_location,__LINE__){ name, __FUNCTION__,  __FILE__, static_cast<uint32_t>(__LINE__), 0 }; \
    TRACY_GPU_SCOPE_BEGIN_AT_LOCATION(gpu_scope, &TracyConcat(__tracy_gpu_source_location,__LINE__))

#define TRACY_GPU_SCOPE_TRY_BEGIN(gpu_scope, name) \
    static const tracy::SourceLocationData TracyConcat(__tracy_gpu_source_location,__LINE__){ name, __FUNCTION__,  __FILE__, static_cast<uint32_t>(, 0 }; \
    TRACY_GPU_SCOPE_TRY_BEGIN_AT_LOCATION(gpu_scope, &TracyConcat(__tracy_gpu_source_location,__LINE__))

#define TRACY_GPU_SCOPE_END(gpu_scope) \
    gpu_scope.End()

#define TRACY_GPU_SCOPE_COMPLETE(gpu_scope, gpu_time_range) \
    const auto gpu_time_range_var = gpu_time_range; \
    gpu_scope.Complete(static_cast<Timestamp>(gpu_time_range_var.GetStart()), static_cast<Timestamp>(gpu_time_range_var.GetEnd()))

#else // TRACY_GPU_ENABLE

    explicit GpuScope(GpuContext&) { }

    inline void Begin(const void*, int) { }
    inline void End() { }
    inline void Complete(Timestamp, Timestamp) { }
};

} // namespace Methane::Tracy

struct SourceLocationStub { };

#define TRACE_SOURCE_LOCATION_TYPE SourceLocationStub
#define CREATE_TRACY_SOURCE_LOCATION(name) new TRACE_SOURCE_LOCATION_TYPE{}
#define TRACY_GPU_SCOPE_TYPE void*
#define TRACY_GPU_SCOPE_INIT(gpu_context) nullptr
#define TRACY_GPU_SCOPE_BEGIN_AT_LOCATION(gpu_scope, p_location)
#define TRACY_GPU_SCOPE_TRY_BEGIN_AT_LOCATION(gpu_scope, p_location)
#define TRACY_GPU_SCOPE_BEGIN(gpu_scope, name)
#define TRACY_GPU_SCOPE_TRY_BEGIN(gpu_scope, name)
#define TRACY_GPU_SCOPE_END(gpu_scope)
#define TRACY_GPU_SCOPE_COMPLETE(gpu_scope, gpu_time_range)

#endif // TRACY_GPU_ENABLE