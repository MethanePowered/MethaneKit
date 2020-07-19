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
    META_FUNCTION_TASK();
    return std::make_shared<RenderContextVK>(env, static_cast<DeviceBase&>(device), settings);
}

RenderContextVK::RenderContextVK(const Platform::AppEnvironment& /*env*/, DeviceBase& device, const RenderContext::Settings& settings)
    : ContextVK<RenderContextBase>(device, settings)
{
    META_FUNCTION_TASK();
}

RenderContextVK::~RenderContextVK()
{
    META_FUNCTION_TASK();
}

void RenderContextVK::Release()
{
    META_FUNCTION_TASK();
    ContextVK<RenderContextBase>::Release();
}

void RenderContextVK::Initialize(DeviceBase& device, bool deferred_heap_allocation, bool is_callback_emitted)
{
    META_FUNCTION_TASK();
    ContextVK<RenderContextBase>::Initialize(device, deferred_heap_allocation, is_callback_emitted);
}

bool RenderContextVK::ReadyToRender() const
{
    META_FUNCTION_TASK();
    return true;
}

void RenderContextVK::WaitForGpu(Context::WaitFor wait_for)
{
    META_FUNCTION_TASK();
    ContextVK<RenderContextBase>::WaitForGpu(wait_for);
}

void RenderContextVK::Resize(const FrameSize& frame_size)
{
    META_FUNCTION_TASK();
    ContextVK<RenderContextBase>::Resize(frame_size);
}

void RenderContextVK::Present()
{
    META_FUNCTION_TASK();
    ContextVK<RenderContextBase>::Present();
    // ...
    ContextVK<RenderContextBase>::OnCpuPresentComplete();
}

bool RenderContextVK::SetVSyncEnabled(bool vsync_enabled)
{
    META_FUNCTION_TASK();
    return RenderContextBase::SetVSyncEnabled(vsync_enabled);
}

bool RenderContextVK::SetFrameBuffersCount(uint32_t frame_buffers_count)
{
    META_FUNCTION_TASK();
    return RenderContextBase::SetFrameBuffersCount(frame_buffers_count);
}

float RenderContextVK::GetContentScalingFactor() const
{
    META_FUNCTION_TASK();
    return 1.f;
}

uint32_t RenderContextVK::GetFontResolutionDpi() const
{
    META_FUNCTION_TASK();
    return 96u;
}

CommandQueueVK& RenderContextVK::GetRenderCommandQueueVK()
{
    META_FUNCTION_TASK();
    return static_cast<CommandQueueVK&>(ContextVK<RenderContextBase>::GetRenderCommandQueue());
}

} // namespace Methane::Graphics
