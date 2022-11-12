/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/DirectX/Fence.cpp
DirectX 12 fence implementation.

******************************************************************************/

#include <Methane/Graphics/DirectX/Fence.h>
#include <Methane/Graphics/DirectX/CommandQueue.h>
#include <Methane/Graphics/DirectX/Device.h>
#include <Methane/Graphics/DirectX/IContextDx.h>

#include <Methane/Graphics/Base/Context.h>
#include <Methane/Instrumentation.h>
#include <Methane/Graphics/Windows/DirectXErrorHandling.h>

#include <nowide/convert.hpp>

namespace Methane::Graphics
{

Ptr <IFence> IFence::Create(ICommandQueue& command_queue)
{
    META_FUNCTION_TASK();
    return std::make_shared<DirectX::Fence>(static_cast<Base::CommandQueue&>(command_queue));
}

} // namespace Methane::Graphics

namespace Methane::Graphics::DirectX
{

Fence::Fence(Base::CommandQueue& command_queue)
    : Base::Fence(command_queue)
    , m_event(CreateEvent(nullptr, FALSE, FALSE, nullptr))
{
    META_FUNCTION_TASK();
    if (!m_event)
    {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }

    const wrl::ComPtr<ID3D12Device>& cp_device = GetDirectCommandQueue().GetDirectContext().GetDirectDevice().GetNativeDevice();
    META_CHECK_ARG_NOT_NULL(cp_device);

    ThrowIfFailed(cp_device->CreateFence(GetValue(), D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_cp_fence)), cp_device.Get());
}

Fence::~Fence()
{
    META_FUNCTION_TASK();
    SafeCloseHandle(m_event);
}

void Fence::Signal()
{
    META_FUNCTION_TASK();
    Base::Fence::Signal();

    META_CHECK_ARG_NOT_NULL(m_cp_fence);
    CommandQueue& command_queue = GetDirectCommandQueue();
    ThrowIfFailed(command_queue.GetNativeCommandQueue().Signal(m_cp_fence.Get(), GetValue()),
                  command_queue.GetDirectContext().GetDirectDevice().GetNativeDevice().Get());
}

void Fence::WaitOnCpu()
{
    META_FUNCTION_TASK();
    Base::Fence::WaitOnCpu();

    const uint64_t wait_value = GetValue();
    const uint64_t curr_value = m_cp_fence->GetCompletedValue();
    if (curr_value >= wait_value) // NOSONAR - curr_value declared outside if
        return;

    META_LOG("Fence '{}' with value {} SLEEP until value {}", GetName(), curr_value, wait_value);

    META_CHECK_ARG_NOT_NULL(m_cp_fence);
    META_CHECK_ARG_NOT_NULL(m_event);

    ThrowIfFailed(m_cp_fence->SetEventOnCompletion(GetValue(), m_event),
                  GetDirectCommandQueue().GetDirectContext().GetDirectDevice().GetNativeDevice().Get());
    WaitForSingleObjectEx(m_event, INFINITE, FALSE);

    META_LOG("Fence '{}' AWAKE on value {}", GetName(), wait_value);
}

void Fence::WaitOnGpu(ICommandQueue& wait_on_command_queue)
{
    META_FUNCTION_TASK();
    Base::Fence::WaitOnGpu(wait_on_command_queue);

    META_CHECK_ARG_NOT_NULL(m_cp_fence);
    auto& dx_wait_on_command_queue = static_cast<CommandQueue&>(wait_on_command_queue);
    ID3D12CommandQueue& native_wait_on_command_queue = dx_wait_on_command_queue.GetNativeCommandQueue();
    ThrowIfFailed(native_wait_on_command_queue.Wait(m_cp_fence.Get(), GetValue()),
                  dx_wait_on_command_queue.GetDirectContext().GetDirectDevice().GetNativeDevice().Get());
}

bool Fence::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (!Base::Fence::SetName(name))
        return false;

    META_CHECK_ARG_NOT_NULL(m_cp_fence);
    m_cp_fence->SetName(nowide::widen(name).c_str());
    return true;
}

CommandQueue& Fence::GetDirectCommandQueue()
{
    META_FUNCTION_TASK();
    return static_cast<CommandQueue&>(GetCommandQueue());
}

} // namespace Methane::Graphics::DirectX
