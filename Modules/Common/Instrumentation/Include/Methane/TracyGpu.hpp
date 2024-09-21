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

FILE: Methane/TracyGpu.hpp
Tracy GPU instrumentation helpers

******************************************************************************/

#pragma once

#if defined(TRACY_GPU_ENABLE) && defined(METHANE_GPU_INSTRUMENTATION_ENABLED) && METHANE_GPU_INSTRUMENTATION_ENABLED == 1
#define METHANE_TRACY_GPU_ENABLED
#endif

#ifdef METHANE_TRACY_GPU_ENABLED
#include <tracy/Tracy.hpp>
#include <client/TracyProfiler.hpp>
#include <client/TracyCallstack.hpp>
#include <common/TracyAlloc.hpp>
#endif

#include <Methane/Checks.hpp>

#include <stdlib.h>
#include <string_view>
#include <mutex>

namespace Methane::Tracy
{

class GpuScope;

using QueryId   = uint16_t;
using Timestamp = int64_t;
using ThreadId  = uint32_t;

class GpuContext
{
    friend class GpuScope;

public:
    enum class Type : uint8_t
    {
        Undefined = 0,
        DirectX12,
        Vulkan,
        Metal,
    };

    struct Settings
    {
        Type      type              = Type::Undefined;
        Timestamp gpu_timestamp     = 0;
        Timestamp cpu_timestamp     = 0;
        Timestamp cpu_ref_timestamp = 0;
        float     gpu_time_period   = 1.F; // number of nanoseconds required for a timestamp query to be incremented by 1
        bool      is_thread_local   = false;

#ifdef METHANE_TRACY_GPU_ENABLED

        explicit Settings(Type type = Type::Undefined)
            : type(type)
            , gpu_timestamp(tracy::Profiler::GetTime())
            , cpu_timestamp(gpu_timestamp)
            , cpu_ref_timestamp(cpu_timestamp)
        { }

        Settings(Type type, Timestamp cpu_timestamp, Timestamp gpu_timestamp, float gpu_time_period = 1.F, bool is_thread_local = false)
            : type(type)
            , gpu_timestamp(gpu_timestamp)
            , cpu_timestamp(cpu_timestamp)
            , cpu_ref_timestamp(tracy::Profiler::GetTime())
            , gpu_time_period(gpu_time_period)
            , is_thread_local(is_thread_local)
        { }

#else // METHANE_TRACY_GPU_ENABLED

        Settings() = default;
        explicit Settings(Type)
        {
            // Intentionally unimplemented: stub
        }
        Settings(Type, Timestamp, Timestamp)
        {
            // Intentionally unimplemented: stub
        }
        Settings(Type, Timestamp, Timestamp, float)
        {
            // Intentionally unimplemented: stub
        }
        Settings(Type, Timestamp, Timestamp, float, bool)
        {
            // Intentionally unimplemented: stub
        }

#endif // METHANE_TRACY_GPU_ENABLED
    };

#ifdef METHANE_TRACY_GPU_ENABLED

    static tracy::GpuContextType GetTracyGpuContextType(Type type)
    {
        switch(type)
        {
        case Type::DirectX12: return tracy::GpuContextType::Direct3D12;
        case Type::Vulkan:    return tracy::GpuContextType::Vulkan;
        case Type::Metal:     return tracy::GpuContextType::Invalid;
        case Type::Undefined: return tracy::GpuContextType::Invalid;
        default:              META_UNEXPECTED_RETURN(type, tracy::GpuContextType::Invalid);
        }
    }

    explicit GpuContext(const Settings& settings)
        : m_id(tracy::GetGpuCtxCounter().fetch_add( 1, std::memory_order_relaxed ))
        , m_prev_calibration_cpu_timestamp(settings.cpu_timestamp)
    {
        META_CHECK_LESS_DESCR(m_id, 255, "Tracy GPU context count is exceeding the maximum 255.");

        auto item = tracy::Profiler::QueueSerial();
        tracy::MemWrite(&item->hdr.type,              tracy::QueueType::GpuNewContext);
        tracy::MemWrite(&item->gpuNewContext.cpuTime, settings.cpu_ref_timestamp);
        tracy::MemWrite(&item->gpuNewContext.gpuTime, settings.gpu_timestamp);
        tracy::MemWrite(&item->gpuNewContext.period,  settings.gpu_time_period);
        tracy::MemWrite(&item->gpuNewContext.context, m_id);
        tracy::MemWrite(&item->gpuNewContext.flags,   tracy::GpuContextCalibration);
        tracy::MemWrite(&item->gpuNewContext.type,    GetTracyGpuContextType(settings.type));
        if (settings.is_thread_local)
        {
            const auto thread = tracy::GetThreadHandle();
            tracy::MemWrite(&item->gpuNewContext.thread, thread);
        }
        else
        {
            memset(&item->gpuNewContext.thread, 0, sizeof(item->gpuNewContext.thread));
        }

        FinishSerialItem(*item);
    }

    void Calibrate(Timestamp cpu_timestamp, Timestamp gpu_timestamp) noexcept
    {
        const int64_t reference_cpu_timestamp = tracy::Profiler::GetTime();
        const int64_t cpu_delta = static_cast<int64_t>(cpu_timestamp) - m_prev_calibration_cpu_timestamp;
        if (cpu_delta <= 0)
            return;

        m_prev_calibration_cpu_timestamp = cpu_timestamp;
        auto item = tracy::Profiler::QueueSerial();
        tracy::MemWrite( &item->hdr.type, tracy::QueueType::GpuCalibration );
        tracy::MemWrite( &item->gpuCalibration.gpuTime,  gpu_timestamp );
        tracy::MemWrite( &item->gpuCalibration.cpuTime,  reference_cpu_timestamp );
        tracy::MemWrite( &item->gpuCalibration.cpuDelta, cpu_delta );
        tracy::MemWrite( &item->gpuCalibration.context,  m_id );
        tracy::Profiler::QueueSerialFinish();
    }

    void SetName(std::string_view name) noexcept
    {
        const auto name_ptr = reinterpret_cast<char*>(tracy::tracy_malloc(name.length())); // NOSONAR
        memcpy(name_ptr, name.data(), name.length());

        auto item = tracy::Profiler::QueueSerial();
        tracy::MemWrite(&item->hdr.type,                  tracy::QueueType::GpuContextName);
        tracy::MemWrite(&item->gpuContextNameFat.context, m_id);
        tracy::MemWrite(&item->gpuContextNameFat.ptr,     reinterpret_cast<uint64_t>(name_ptr)); // NOSONAR
        tracy::MemWrite(&item->gpuContextNameFat.size,    static_cast<uint16_t>(name.length()));

        FinishSerialItem(*item);
    }

private:
    tracy_force_inline void FinishSerialItem(const tracy::QueueItem& item) const noexcept
    {
#ifdef TRACY_ON_DEMAND
        tracy::GetProfiler().DeferItem(item);
#else
        (void)item;
#endif
        tracy::Profiler::QueueSerialFinish();
    }

    tracy_force_inline QueryId NextQueryId() noexcept
    {
        std::scoped_lock lock_guard(m_query_mutex);
        m_query_id = (m_query_id + 1) % m_query_count;
        return m_query_id;
    }

    tracy_force_inline uint8_t GetId() const noexcept
    {
        return m_id;
    }

    const uint8_t               m_id;
    const QueryId               m_query_count = std::numeric_limits<QueryId>::max();
    int64_t                     m_prev_calibration_cpu_timestamp = 0;
    QueryId                     m_query_id    = 0U;
    TracyLockable(std::mutex,   m_query_mutex);

#else // METHANE_TRACY_GPU_ENABLED

    explicit GpuContext(const Settings&)                { /* empty method when Tracy GPU is disabled */ }

    void Calibrate(Timestamp, Timestamp) const noexcept { /* empty method when Tracy GPU is disabled */ }

    void SetName(std::string_view) const noexcept       { /* empty method when Tracy GPU is disabled */ }

#endif // METHANE_TRACY_GPU_ENABLED
};

class GpuScope
{
public:
#ifdef METHANE_TRACY_GPU_ENABLED
    enum class State
    {
        Begun,
        Ended,
        Completed
    };

    explicit GpuScope(GpuContext* context_ptr)
        : m_context_ptr(context_ptr)
    {
    }

    tracy_force_inline void Begin(uint64_t src_location, bool is_allocated_location = false, int call_stack_depth = 0)
    {
        if (!m_context_ptr)
            return;

#ifdef TRACY_ON_DEMAND
        m_is_active = tracy::GetProfiler().IsConnected();
        if (!m_is_active)
            return;
#endif

        m_state           = State::Begun;
        m_begin_thread_id = tracy::GetThreadHandle();
        m_begin_query_id  = m_context_ptr->NextQueryId();

        tracy::QueueItem* item = nullptr;
        tracy::QueueType item_type {};
        if (call_stack_depth)
        {
            item = tracy::Profiler::QueueSerialCallstack(tracy::Callstack(call_stack_depth));
            item_type = is_allocated_location
                      ? tracy::QueueType::GpuZoneBeginAllocSrcLocCallstackSerial
                      : tracy::QueueType::GpuZoneBeginCallstackSerial;
        }
        else
        {
            item = tracy::Profiler::QueueSerial();
            item_type = is_allocated_location
                      ? tracy::QueueType::GpuZoneBeginAllocSrcLocSerial
                      : tracy::QueueType::GpuZoneBeginSerial;
        }

        tracy::MemWrite(&item->hdr.type,             item_type);
        tracy::MemWrite(&item->gpuZoneBegin.cpuTime, tracy::Profiler::GetTime());
        tracy::MemWrite(&item->gpuZoneBegin.srcloc,  src_location);
        tracy::MemWrite(&item->gpuZoneBegin.thread,  m_begin_thread_id);
        tracy::MemWrite(&item->gpuZoneBegin.queryId, m_begin_query_id);
        tracy::MemWrite(&item->gpuZoneBegin.context, m_context_ptr->GetId());
        tracy::Profiler::QueueSerialFinish();
    }

    tracy_force_inline void Begin(int line, std::string_view source_file, std::string_view function, int call_stack_depth = 0)
    {
        if (!m_context_ptr)
            return;

#ifdef TRACY_ON_DEMAND
        m_is_active = tracy::GetProfiler().IsConnected();
        if (!m_is_active)
            return;
#endif

        const uint64_t source_location = tracy::Profiler::AllocSourceLocation(static_cast<uint32_t>(line),
                                                                              source_file.data(), source_file.length(),
                                                                              function.data(), function.length());
        Begin(source_location, call_stack_depth);
    }

    tracy_force_inline void Begin(std::string_view name, int line, std::string_view source_file, std::string_view function,  int call_stack_depth = 0)
    {
        if (!m_context_ptr)
            return;

        META_CHECK_NOT_EMPTY(name);
        
#ifdef TRACY_ON_DEMAND
        m_is_active = tracy::GetProfiler().IsConnected();
        if (!m_is_active)
            return;
#endif
        
        const uint64_t source_location = tracy::Profiler::AllocSourceLocation(static_cast<uint32_t>(line),
                                                                              source_file.data(), source_file.length(),
                                                                              function.data(), function.length(),
                                                                              name.data(), name.length());
        Begin(source_location, true, call_stack_depth);
    }

    tracy_force_inline void End()
    {
        if (!m_context_ptr)
            return;

#ifdef TRACY_ON_DEMAND
        if (!m_is_active)
            return;
#endif

        META_CHECK_EQUAL_DESCR(m_state, State::Begun, "GPU scope can end only from begun states");
        m_state        = State::Ended;
        m_end_query_id = m_context_ptr->NextQueryId();

        auto item = tracy::Profiler::QueueSerial();
        tracy::MemWrite(&item->hdr.type,           tracy::QueueType::GpuZoneEndSerial);
        tracy::MemWrite(&item->gpuZoneEnd.cpuTime, tracy::Profiler::GetTime());
        tracy::MemWrite(&item->gpuZoneEnd.thread,  m_begin_thread_id);
        tracy::MemWrite(&item->gpuZoneEnd.queryId, m_end_query_id);
        tracy::MemWrite(&item->gpuZoneEnd.context, m_context_ptr->GetId());
        tracy::Profiler::QueueSerialFinish();
    }

    tracy_force_inline void Complete(Timestamp gpu_begin_timestamp, Timestamp gpu_end_timestamp)
    {
        if (!m_context_ptr || gpu_begin_timestamp == gpu_end_timestamp)
            return;

#ifdef TRACY_ON_DEMAND
        if (!m_is_active)
            return;
#endif

        META_CHECK_EQUAL_DESCR(m_state, State::Ended, "GPU scope can be completed only from ended state");
        META_CHECK_RANGE_INC_DESCR(gpu_begin_timestamp, Timestamp(0), gpu_end_timestamp,
                                       "GPU begin timestamp should be less or equal to end timestamp and both should be positive");
        m_state = State::Completed;

        auto begin_item = tracy::Profiler::QueueSerial();
        tracy::MemWrite(&begin_item->hdr.type, tracy::QueueType::GpuTime);
        tracy::MemWrite(&begin_item->gpuTime.gpuTime, gpu_begin_timestamp);
        tracy::MemWrite(&begin_item->gpuTime.queryId, m_begin_query_id);
        tracy::MemWrite(&begin_item->gpuTime.context, m_context_ptr->GetId());
        tracy::Profiler::QueueSerialFinish();

        auto end_item = tracy::Profiler::QueueSerial();
        tracy::MemWrite(&end_item->hdr.type, tracy::QueueType::GpuTime);
        tracy::MemWrite(&end_item->gpuTime.gpuTime, gpu_end_timestamp);
        tracy::MemWrite(&end_item->gpuTime.queryId, m_end_query_id);
        tracy::MemWrite(&end_item->gpuTime.context, m_context_ptr->GetId());
        tracy::Profiler::QueueSerialFinish();
    }

    tracy_force_inline State GetState() const { return m_state; }

private:
    GpuContext* const m_context_ptr;
    State             m_state           = State::Completed;
    ThreadId          m_begin_thread_id = 0U;
    QueryId           m_begin_query_id  = 0U;
    QueryId           m_end_query_id    = 0U;

#ifdef TRACY_ON_DEMAND
    bool m_is_active = true;
#endif
};

} // namespace Methane::Tracy

#define TRACY_SOURCE_LOCATION_ALLOC_UNNAMED() \
    tracy::Profiler::AllocSourceLocation(static_cast<uint32_t>(__LINE__), __FILE__, __FUNCTION__)

#define TRACY_SOURCE_LOCATION_ALLOC(name) \
    tracy::Profiler::AllocSourceLocation(static_cast<uint32_t>(__LINE__), __FILE__, __FUNCTION__, name.c_str(), name.length())

#define TRACY_GPU_SCOPE_TYPE Methane::Tracy::GpuScope
#define TRACY_GPU_SCOPE_INIT(gpu_context) gpu_context

#define TRACY_GPU_SCOPE_BEGIN_AT_LOCATION(gpu_scope, location_ptr) \
    gpu_scope.Begin(location_ptr)

#define TRACY_GPU_SCOPE_BEGIN_UNNAMED(gpu_scope) \
    gpu_scope.Begin(__LINE__, __FUNCTION__,  __FILE__)

#define TRACY_GPU_SCOPE_BEGIN_NAMED(gpu_scope, name) \
    gpu_scope.Begin(name, __LINE__, __FUNCTION__,  __FILE__)

#define TRACY_GPU_SCOPE_TRY_BEGIN_AT_LOCATION(gpu_scope, location_ptr) \
    if (gpu_scope.GetState() != Methane::Tracy::GpuScope::State::Begun) \
        gpu_scope.Begin(location_ptr)

#define TRACY_GPU_SCOPE_TRY_BEGIN_UNNAMED(gpu_scope) \
    if (gpu_scope.GetState() != Methane::Tracy::GpuScope::State::Begun) \
        gpu_scope.Begin(__LINE__, __FUNCTION__,  __FILE__)

#define TRACY_GPU_SCOPE_TRY_BEGIN_NAMED(gpu_scope, name) \
    if (gpu_scope.GetState() != Methane::Tracy::GpuScope::State::Begun) \
        gpu_scope.Begin(name, __LINE__, __FUNCTION__,  __FILE__)

#define TRACY_GPU_SCOPE_BEGIN(gpu_scope, name) \
    static const tracy::SourceLocationData TracyConcat(__tracy_gpu_source_location,__LINE__){ name, __FUNCTION__,  __FILE__, static_cast<uint32_t>(__LINE__), 0U }; \
    TRACY_GPU_SCOPE_BEGIN_AT_LOCATION(gpu_scope, reinterpret_cast<uint64_t>(&TracyConcat(__tracy_gpu_source_location,__LINE__)) /* NOSONAR */)

#define TRACY_GPU_SCOPE_TRY_BEGIN(gpu_scope, name) \
    static const tracy::SourceLocationData TracyConcat(__tracy_gpu_source_location,__LINE__){ name, __FUNCTION__,  __FILE__, static_cast<uint32_t>(__LINE__), 0U }; \
    TRACY_GPU_SCOPE_TRY_BEGIN_AT_LOCATION(gpu_scope, reinterpret_cast<uint64_t>(&TracyConcat(__tracy_gpu_source_location,__LINE__)) /* NOSONAR */)

#define TRACY_GPU_SCOPE_END(gpu_scope) \
    gpu_scope.End()

#define TRACY_GPU_SCOPE_COMPLETE(gpu_scope, gpu_time_range) \
    const auto gpu_time_range_var = gpu_time_range; \
    gpu_scope.Complete(static_cast<Methane::Data::Timestamp>(gpu_time_range_var.GetStart()), static_cast<Methane::Data::Timestamp>(gpu_time_range_var.GetEnd()))

#else // METHANE_TRACY_GPU_ENABLED

    explicit GpuScope(const GpuContext&)
    {
        // Intentionally unimplemented: stub
    }

    inline void Begin(const char*, int) const
    {
        // Intentionally unimplemented: stub
    }
    inline void End() const
    {
        // Intentionally unimplemented: stub
    }
    inline void Complete(Timestamp, Timestamp) const
    {
        // Intentionally unimplemented: stub
    }
};

} // namespace Methane::Tracy

struct SourceLocationStub { };

#define TRACY_SOURCE_LOCATION_ALLOC_UNNAMED() 0U
#define TRACY_SOURCE_LOCATION_ALLOC(name) 0U
#define TRACY_GPU_SCOPE_TYPE char* /* NOSONAR */
#define TRACY_GPU_SCOPE_INIT(gpu_context) nullptr
#define TRACY_GPU_SCOPE_BEGIN_AT_LOCATION(gpu_scope, location_ptr)
#define TRACY_GPU_SCOPE_BEGIN_UNNAMED(gpu_scope)
#define TRACY_GPU_SCOPE_BEGIN_NAMED(gpu_scope, name)
#define TRACY_GPU_SCOPE_TRY_BEGIN_AT_LOCATION(gpu_scope, location_ptr)
#define TRACY_GPU_SCOPE_TRY_BEGIN_UNNAMED(gpu_scope)
#define TRACY_GPU_SCOPE_TRY_BEGIN_NAMED(gpu_scope, name)
#define TRACY_GPU_SCOPE_BEGIN(gpu_scope, name)
#define TRACY_GPU_SCOPE_TRY_BEGIN(gpu_scope, name)
#define TRACY_GPU_SCOPE_END(gpu_scope)
#define TRACY_GPU_SCOPE_COMPLETE(gpu_scope, gpu_time_range)

#endif // METHANE_TRACY_GPU_ENABLED
