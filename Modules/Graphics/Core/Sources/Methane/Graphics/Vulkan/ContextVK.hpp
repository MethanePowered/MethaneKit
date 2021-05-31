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

FILE: Methane/Graphics/Metal/ContextVK.hpp
Vulkan template implementation of the base context interface.

******************************************************************************/

#pragma once

#include "ContextVK.h"
#include "DeviceVK.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Graphics/RenderContext.h>
#include <Methane/Graphics/CommandKit.h>
#include <Methane/Instrumentation.h>

#include <string>
#include <map>

namespace Methane::Graphics
{

struct CommandQueue;

template<class ContextBaseT, typename = std::enable_if_t<std::is_base_of_v<ContextBase, ContextBaseT>>>
class ContextVK : public ContextBaseT
{
public:
    ContextVK(DeviceBase& device, tf::Executor& parallel_executor, const typename ContextBaseT::Settings& settings)
        : ContextBaseT(device, parallel_executor, settings)
    {
        META_FUNCTION_TASK();
    }

    // Context interface

    void WaitForGpu(Context::WaitFor wait_for) override
    {
        META_FUNCTION_TASK();
        ContextBase::WaitForGpu(wait_for);
        // ...
        ContextBase::OnGpuWaitComplete(wait_for);
    }

    // ContextBase interface

    void Initialize(DeviceBase& device, bool deferred_heap_allocation, bool is_callback_emitted = true) override
    {
        META_FUNCTION_TASK();
        ContextBase::Initialize(device, deferred_heap_allocation, is_callback_emitted);
    }

    void Release() override
    {
        META_FUNCTION_TASK();
        ContextBase::Release();
    }

    // IContextVK interface

    const DeviceVK& GetDeviceVK() const noexcept final
    {
        META_FUNCTION_TASK();
        return dynamic_cast<const DeviceVK&>(ContextBase::GetDeviceBase());
    }

    CommandQueueVK& GetDefaultCommandQueueVK(CommandList::Type type) final
    {
        META_FUNCTION_TASK();
        return dynamic_cast<CommandQueueVK&>(ContextBase::GetDefaultCommandKit(type).GetQueue());
    }
};

} // namespace Methane::Graphics
