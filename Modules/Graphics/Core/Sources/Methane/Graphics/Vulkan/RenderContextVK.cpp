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

FILE: Methane/Graphics/Metal/RenderContextVK.mm
Vulkan implementation of the render context interface.

******************************************************************************/

#include "RenderContextVK.h"
#include "DeviceVK.h"
#include "RenderStateVK.h"
#include "CommandQueueVK.h"
#include "TypesVK.h"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

Ptr<RenderContext> RenderContext::Create(const Platform::AppEnvironment& env, Device& device, const RenderContext::Settings& settings)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<RenderContextVK>(env, static_cast<DeviceBase&>(device), settings);
}

RenderContextVK::RenderContextVK(const Platform::AppEnvironment& env, DeviceBase& device, const RenderContext::Settings& settings)
    : ContextVK<RenderContextBase>(device, settings)
{
    ITT_FUNCTION_TASK();
}

RenderContextVK::~RenderContextVK()
{
    ITT_FUNCTION_TASK();
}

void RenderContextVK::Release()
{
    ITT_FUNCTION_TASK();
    ContextVK<RenderContextBase>::Release();
}

void RenderContextVK::Initialize(DeviceBase& device, bool deferred_heap_allocation)
{
    ITT_FUNCTION_TASK();
    ContextVK<RenderContextBase>::Initialize(device, deferred_heap_allocation);
}

bool RenderContextVK::ReadyToRender() const
{
    ITT_FUNCTION_TASK();
    return true;
}

void RenderContextVK::WaitForGpu(Context::WaitFor wait_for)
{
    ITT_FUNCTION_TASK();
    ContextVK<RenderContextBase>::WaitForGpu(wait_for);
}

void RenderContextVK::Resize(const FrameSize& frame_size)
{
    ITT_FUNCTION_TASK();
    ContextVK<RenderContextBase>::Resize(frame_size);
}

void RenderContextVK::Present()
{
    ITT_FUNCTION_TASK();
    ContextVK<RenderContextBase>::Present();
    // ...
    ContextVK<RenderContextBase>::OnCpuPresentComplete();
}

bool RenderContextVK::SetVSyncEnabled(bool vsync_enabled)
{
    ITT_FUNCTION_TASK();
    return false;
}

bool RenderContextVK::SetFrameBuffersCount(uint32_t frame_buffers_count)
{
    ITT_FUNCTION_TASK();
    return false;
}

float RenderContextVK::GetContentScalingFactor() const
{
    ITT_FUNCTION_TASK();
    return 1.f;
}
    
CommandQueueVK& RenderContextVK::GetRenderCommandQueueVK()
{
    ITT_FUNCTION_TASK();
    return static_cast<CommandQueueVK&>(ContextVK<RenderContextBase>::GetRenderCommandQueue());
}

} // namespace Methane::Graphics
