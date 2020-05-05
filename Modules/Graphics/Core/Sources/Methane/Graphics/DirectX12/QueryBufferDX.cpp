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

#include <Methane/Graphics/RenderContext.h>
#include <Methane/Graphics/QueryBuffer.h>
#include <Methane/Graphics/ContextBase.h>
#include <Methane/Graphics/Windows/Primitives.h>
#include <Methane/Instrumentation.h>

#include <wrl.h>
#include <d3d12.h>

namespace wrl = Microsoft::WRL;

namespace Methane::Graphics
{

QueryBufferDX::QueryBufferDX(CommandQueueDX& command_queue, Type type, Data::Size buffer_size)
    : QueryBuffer(static_cast<CommandQueueBase&>(command_queue), type)
    , m_sp_result_buffer(Buffer::CreateReadBackBuffer(GetContext(), buffer_size))
    , m_result_resource_dx(dynamic_cast<const ResourceDX&>(*m_sp_result_buffer))
    , m_context_dx(dynamic_cast<IContextDX&>(GetContext()))
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

static Data::Size GetTimestampResultBufferSize(const Context& context, uint32_t max_timestamps_per_frame)
{
    const uint32_t frames_count = context.GetType() == Context::Type::Render
                                ? dynamic_cast<const RenderContext&>(context).GetSettings().frame_buffers_count
                                : 1u;
    return frames_count * max_timestamps_per_frame * sizeof(GpuTimestamp);
}

TimestampQueryBufferDX::TimestampQueryBufferDX(CommandQueueDX& command_queue, uint32_t max_timestamps_per_frame)
    : QueryBufferDX(command_queue, Type::Timestamp, GetTimestampResultBufferSize(command_queue.GetContext(), max_timestamps_per_frame))
    , m_max_timestamps_per_frame(max_timestamps_per_frame)
    , m_gpu_frequency(Graphics::GetGpuFrequency(GetCommandQueueDX().GetNativeCommandQueue(), *GetContextDX().GetDeviceDX().GetNativeDevice().Get()))
{
    META_FUNCTION_TASK();
}

} // namespace Methane::Graphics
