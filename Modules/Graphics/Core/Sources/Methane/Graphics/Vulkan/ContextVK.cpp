/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/ContextVK.mm
Vulkan implementation of the context interface.

******************************************************************************/

#include "ContextVK.h"
#include "DeviceVK.h"
#include "CommandQueueVK.h"
#include "TypesVK.h"

#include <Methane/Data/Instrumentation.h>

namespace Methane::Graphics
{

Context::Ptr Context::Create(const Platform::AppEnvironment& env, Device& device, const Context::Settings& settings)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ContextVK>(env, static_cast<DeviceBase&>(device), settings);
}

ContextVK::ContextVK(const Platform::AppEnvironment& env, DeviceBase& device, const Context::Settings& settings)
    : ContextBase(device, settings)
{
    ITT_FUNCTION_TASK();
    m_resource_manager.Initialize({ true });
}

ContextVK::~ContextVK()
{
    ITT_FUNCTION_TASK();
}

void ContextVK::Release()
{
    ITT_FUNCTION_TASK();

    ContextBase::Release();
}

void ContextVK::Initialize(Device& device, bool deferred_heap_allocation)
{
    ITT_FUNCTION_TASK();

    ContextBase::Initialize(device, deferred_heap_allocation);
}

bool ContextVK::ReadyToRender() const
{
    ITT_FUNCTION_TASK();
    return true;
}

void ContextVK::OnCommandQueueCompleted(CommandQueue& /*cmd_queue*/, uint32_t /*frame_index*/)
{
    ITT_FUNCTION_TASK();
}

void ContextVK::WaitForGpu(WaitFor wait_for)
{
    ITT_FUNCTION_TASK();

    const bool switch_to_next_frame = (wait_for == WaitFor::FramePresented);
    ContextBase::WaitForGpu(wait_for);

    if (switch_to_next_frame)
    {
        m_frame_buffer_index = (m_frame_buffer_index + 1) % m_settings.frame_buffers_count;
    }
}

void ContextVK::Resize(const FrameSize& frame_size)
{
    ITT_FUNCTION_TASK();
    ContextBase::Resize(frame_size);
}

void ContextVK::Present()
{
    ITT_FUNCTION_TASK();
    OnPresentComplete();
}

bool ContextVK::SetVSyncEnabled(bool vsync_enabled)
{
    ITT_FUNCTION_TASK();
    return ContextBase::SetVSyncEnabled(vsync_enabled);
}

bool ContextVK::SetFrameBuffersCount(uint32_t frame_buffers_count)
{
    ITT_FUNCTION_TASK();
    return ContextBase::SetFrameBuffersCount(frame_buffers_count);
}

DeviceVK& ContextVK::GetDeviceVK()
{
    ITT_FUNCTION_TASK();
    return static_cast<DeviceVK&>(GetDevice());
}

} // namespace Methane::Graphics
