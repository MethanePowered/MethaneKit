/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/ContextVK.hpp
Vulkan template implementation of the base context interface.

******************************************************************************/

#pragma once

#include "ContextVK.h"
#include "DeviceVK.h"
#include "CommandQueueVK.h"
#include "ResourceManagerVK.h"

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

    ResourceManager& GetResourceManager() noexcept override { return m_resource_manager; }

    void WaitForGpu(Context::WaitFor wait_for) override
    {
        META_FUNCTION_TASK();
        ContextBaseT::WaitForGpu(wait_for);
        // ...
        ContextBaseT::OnGpuWaitComplete(wait_for);
    }

    // IContextVK interface

    const DeviceVK& GetDeviceVK() const noexcept final
    {
        META_FUNCTION_TASK();
        return static_cast<const DeviceVK&>(ContextBaseT::GetDeviceBase());
    }

    CommandQueueVK& GetDefaultCommandQueueVK(CommandList::Type type) final
    {
        META_FUNCTION_TASK();
        return dynamic_cast<CommandQueueVK&>(ContextBaseT::GetDefaultCommandKit(type).GetQueue());
    }

private:
    ResourceManagerVK m_resource_manager;
};

} // namespace Methane::Graphics
