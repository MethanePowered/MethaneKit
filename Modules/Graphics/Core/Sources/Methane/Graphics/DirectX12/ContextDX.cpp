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

FILE: Methane/Graphics/DirectX12/ContextDX.cpp
DirectX 12 implementation of the context interface.

******************************************************************************/

#include "ContextDX.h"
#include "DeviceDX.h"
#include "CommandQueueDX.h"

#include <Methane/Instrumentation.h>
#include <Methane/ScopeTimer.h>
#include <Methane/Graphics/Windows/Helpers.h>

#ifdef COMMAND_EXECUTION_LOGGING
#include <Methane/Platform/Utils.h>
#endif

#include <nowide/convert.hpp>
#include <cassert>

namespace Methane::Graphics
{

ContextDX::ContextDX(DeviceBase& device, Type type)
    : ContextBase(device, type)
{
    ITT_FUNCTION_TASK();
}

ContextDX::~ContextDX()
{
    ITT_FUNCTION_TASK();
}

void ContextDX::Release()
{
    ITT_FUNCTION_TASK();

    m_sp_upload_fence.reset();

    if (m_sp_device)
    {
        static_cast<DeviceDX&>(*m_sp_device).ReleaseNativeDevice();
        m_sp_device.reset();
    }

    ContextBase::Release();

    static_cast<SystemDX&>(System::Get()).ReportLiveObjects();
}

void ContextDX::Initialize(DeviceBase& device, bool deferred_heap_allocation)
{
    ITT_FUNCTION_TASK();

    ContextBase::Initialize(device, deferred_heap_allocation);

    m_sp_upload_fence = std::make_unique<FenceDX>(GetUploadCommandQueueDX());
}

void ContextDX::OnCommandQueueCompleted(CommandQueue& /*cmd_queue*/, uint32_t /*frame_index*/)
{
    ITT_FUNCTION_TASK();
}

void ContextDX::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();
    ContextBase::SetName(name);

    GetDevice().SetName(name + " Device");

    m_sp_upload_fence->SetName(name + " Upload Fence");
}

const DeviceDX& ContextDX::GetDeviceDX() const
{
    ITT_FUNCTION_TASK();
    return static_cast<const DeviceDX&>(GetDeviceBase());
}

void ContextDX::WaitForGpu(WaitFor wait_for)
{
    ITT_FUNCTION_TASK();

    ContextBase::WaitForGpu(wait_for);

    if (wait_for == WaitFor::ResourcesUploaded)
    {
        SCOPE_TIMER("ContextDX::WaitForGpu::ResourcesUploaded");
        assert(!!m_sp_upload_fence);
        m_sp_upload_fence->Flush();
    }

    ContextBase::OnGpuWaitComplete(wait_for);
}

CommandQueueDX& ContextDX::GetUploadCommandQueueDX()
{
    ITT_FUNCTION_TASK();
    return static_cast<CommandQueueDX&>(GetUploadCommandQueue());
}

ContextDX::FenceDX::FenceDX(CommandQueueDX& command_queue)
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

ContextDX::FenceDX::~FenceDX()
{
    ITT_FUNCTION_TASK();
    SafeCloseHandle(m_event);
}

void ContextDX::FenceDX::Signal()
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

void ContextDX::FenceDX::Wait()
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

void ContextDX::FenceDX::Flush()
{
    ITT_FUNCTION_TASK();
    Signal();
    Wait();
}

void ContextDX::FenceDX::SetName(const std::string& name) noexcept
{
    ITT_FUNCTION_TASK();
    if (GetName() == name)
        return;

   ObjectBase::SetName(name);

    assert(!!m_cp_fence);
    m_cp_fence->SetName(nowide::widen(name).c_str());
}

} // namespace Methane::Graphics
