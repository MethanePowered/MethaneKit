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

FILE: Methane/Graphics/DirectX12/FenceDX.cpp
DirectX 12 fence wrapper.

******************************************************************************/

#include "FenceDX.h"
#include "CommandQueueDX.h"
#include "DeviceDX.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>
#include <Methane/ScopeTimer.h>
#include <Methane/Graphics/Windows/Helpers.h>

#include <nowide/convert.hpp>

namespace Methane::Graphics
{


FenceDX::FenceDX(CommandQueueDX& command_queue)
    : m_command_queue(command_queue)
    , m_event(CreateEvent(nullptr, FALSE, FALSE, nullptr))
{
    ITT_FUNCTION_TASK();
    if (!m_event)
    {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }

    const wrl::ComPtr<ID3D12Device>& cp_device = m_command_queue.GetContextDX().GetDeviceDX().GetNativeDevice();
    assert(!!cp_device);

    ThrowIfFailed(cp_device->CreateFence(m_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_cp_fence)));
}

FenceDX::~FenceDX()
{
    ITT_FUNCTION_TASK();
    SafeCloseHandle(m_event);
}

void FenceDX::Signal()
{
    ITT_FUNCTION_TASK();
    wrl::ComPtr<ID3D12CommandQueue>& cp_command_queue = m_command_queue.GetNativeCommandQueue();
    assert(!!cp_command_queue);
    assert(!!m_cp_fence);

    m_value++;

#ifdef COMMAND_EXECUTION_LOGGING
    Platform::PrintToDebugOutput("SIGNAL fence \"" + m_name + "\" with value " + std::to_string(m_value));
#endif

    ThrowIfFailed(cp_command_queue->Signal(m_cp_fence.Get(), m_value));
}

void FenceDX::Wait()
{
    ITT_FUNCTION_TASK();
    assert(!!m_cp_fence);
    assert(!!m_event);

#ifdef COMMAND_EXECUTION_LOGGING
    Platform::PrintToDebugOutput("WAIT fence \"" + m_name + "\" with value " + std::to_string(m_value));
#endif

    if (m_cp_fence->GetCompletedValue() < m_value)
    {
        ThrowIfFailed(m_cp_fence->SetEventOnCompletion(m_value, m_event));
        WaitForSingleObjectEx(m_event, INFINITE, FALSE);
    }
}

void FenceDX::Flush()
{
    ITT_FUNCTION_TASK();
    Signal();
    Wait();
}

void FenceDX::SetName(const std::string& name) noexcept
{
    ITT_FUNCTION_TASK();
    if (GetName() == name)
        return;

   ObjectBase::SetName(name);

    assert(!!m_cp_fence);
    m_cp_fence->SetName(nowide::widen(name).c_str());
}


FrameFenceDX::FrameFenceDX(CommandQueueDX& command_queue, uint32_t frame)
    : FenceDX(command_queue)
    , m_frame(frame)
{
    ITT_FUNCTION_TASK();
}

} // namespace Methane::Graphics
