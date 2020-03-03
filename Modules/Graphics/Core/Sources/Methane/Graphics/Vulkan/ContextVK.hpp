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

FILE: Methane/Graphics/Metal/ContextVK.hpp
Vulkan template implementation of the base context interface.

******************************************************************************/

#pragma once

#include "ContextVK.h"
#include "DeviceVK.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Graphics/RenderContext.h>
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
    ContextVK(DeviceBase& device, const typename ContextBaseT::Settings& settings)
        : ContextBaseT(device, settings)
    {
        ITT_FUNCTION_TASK();
    }

    ~ContextVK() override
    {
        ITT_FUNCTION_TASK();
    }

    // Context interface

    void WaitForGpu(Context::WaitFor wait_for) override
    {
        ITT_FUNCTION_TASK();
        ContextBase::WaitForGpu(wait_for);
        // ...
        ContextBase::OnGpuWaitComplete(wait_for);
    }

    // ContextBase interface

    void Initialize(DeviceBase& device, bool deferred_heap_allocation) override
    {
        ITT_FUNCTION_TASK();
        ContextBase::Initialize(device, deferred_heap_allocation);
    }

    void Release() override
    {
        ITT_FUNCTION_TASK();
        ContextBase::Release();
    }

    // IContextVK interface

    DeviceVK& GetDeviceVK() noexcept override
    {
        ITT_FUNCTION_TASK();
        return dynamic_cast<DeviceVK&>(ContextBase::GetDeviceBase());
    }

    CommandQueueVK& GetUploadCommandQueueVK() noexcept override
    {
        ITT_FUNCTION_TASK();
        return dynamic_cast<CommandQueueVK&>(ContextBase::GetUploadCommandQueue());
    }
};

} // namespace Methane::Graphics
