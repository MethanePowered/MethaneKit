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

FILE: Methane/Graphics/DirectX12/QueryBufferDX.hpp
DirectX 12 GPU query results buffer.

******************************************************************************/

#include "QueryBufferDX.h"
#include "CommandQueueDX.h"
#include "BufferDX.h"
#include "ContextDX.h"
#include "DeviceDX.h"

#include <Methane/Graphics/QueryBuffer.h>
#include <Methane/Graphics/ContextBase.h>
#include <Methane/Graphics/Windows/Primitives.h>
#include <Methane/Instrumentation.h>

#include <wrl.h>
#include <d3d12.h>

namespace wrl = Microsoft::WRL;

namespace Methane::Graphics
{

static D3D12_QUERY_TYPE GetQueryTypeDx(QueryBuffer::Type query_buffer_type)
{
    META_FUNCTION_TASK();
    switch(query_buffer_type)
    {
    case QueryBuffer::Type::Timestamp: return D3D12_QUERY_TYPE_TIMESTAMP;
    //D3D12_QUERY_TYPE_OCCLUSION
    //D3D12_QUERY_TYPE_BINARY_OCCLUSION
    //D3D12_QUERY_TYPE_PIPELINE_STATISTICS
    default: assert(0);
    }
    return D3D12_QUERY_TYPE_TIMESTAMP;
}

static D3D12_QUERY_HEAP_TYPE GetQueryHeapTypeDx(QueryBuffer::Type query_buffer_type)
{
    META_FUNCTION_TASK();
    switch(query_buffer_type)
    {
    case QueryBuffer::Type::Timestamp: return D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    //D3D12_QUERY_HEAP_TYPE_COPY_QUEUE_TIMESTAMP
    //D3D12_QUERY_HEAP_TYPE_OCCLUSION
    //D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS
    default: assert(0);
    }
    return D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
}

QueryBufferDX::QueryBufferDX(CommandQueueDX& command_queue, Type type, Data::Size max_query_count, Data::Size buffer_size, Data::Size query_size)
    : QueryBuffer(static_cast<CommandQueueBase&>(command_queue), type, max_query_count, buffer_size, query_size)
    , m_sp_result_buffer(Buffer::CreateReadBackBuffer(GetContext(), buffer_size))
    , m_context_dx(dynamic_cast<IContextDX&>(GetContext()))
    , m_result_resource_dx(dynamic_cast<ResourceDX&>(*m_sp_result_buffer))
    , m_native_query_type(GetQueryTypeDx(type))
    , m_native_query_heap(m_context_dx.GetNativeQueryHeap(GetQueryHeapTypeDx(type), max_query_count))
{
    META_FUNCTION_TASK();
}

CommandQueueDX& QueryBufferDX::GetCommandQueueDX() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<CommandQueueDX&>(GetCommandQueueBase());
}

static GpuFrequency GetGpuFrequency(ID3D12CommandQueue& native_command_queue, ID3D12Device& native_device)
{
    GpuFrequency gpu_frequency = 0u;
    ThrowIfFailed(native_command_queue.GetTimestampFrequency(&gpu_frequency), &native_device);
    return gpu_frequency;
}

TimestampQueryBufferDX::TimestampQueryBufferDX(CommandQueueDX& command_queue, uint32_t max_timestamps_per_frame)
    : QueryBufferDX(command_queue, Type::Timestamp, 1u << 15u,
                    GetTimestampResultBufferSize(command_queue.GetContext(), max_timestamps_per_frame),
                    sizeof(GpuTimestamp))
    , m_max_timestamps_per_frame(max_timestamps_per_frame)
    , m_gpu_frequency(Graphics::GetGpuFrequency(GetCommandQueueDX().GetNativeCommandQueue(), *GetContextDX().GetDeviceDX().GetNativeDevice().Get()))
{
    META_FUNCTION_TASK();
}

} // namespace Methane::Graphics
