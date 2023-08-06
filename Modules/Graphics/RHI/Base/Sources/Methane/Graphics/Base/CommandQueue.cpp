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

FILE: Methane/Graphics/Base/CommandQueue.cpp
Base implementation of the command queue interface.

******************************************************************************/

#include "Methane/Graphics/RHI/ICommandList.h"
#include <Methane/Graphics/Base/CommandQueue.h>
#include <Methane/Graphics/Base/CommandListSet.h>
#include <Methane/Graphics/Base/CommandKit.h>
#include <Methane/Graphics/Base/RenderContext.h>

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Base
{

CommandQueue::CommandQueue(const Context& context, Rhi::CommandListType command_lists_type)
    : m_context(context)
    , m_device_ptr(context.GetBaseDevicePtr())
    , m_command_lists_type(command_lists_type)
{
    if (context.GetType() == Rhi::ContextType::Compute)
    {
        META_CHECK_ARG_NOT_EQUAL_DESCR(command_lists_type, Rhi::CommandListType::Render,
                                       "compute context can not be used to create render command queues");
    }
    META_CHECK_ARG_NOT_EQUAL_DESCR(command_lists_type, Rhi::CommandListType::ParallelRender,
                                   "command queue should be created with Render type to support ParallelRender command lists");
}

bool CommandQueue::SetName(std::string_view name)
{
    META_FUNCTION_TASK();
    if (!Object::SetName(name))
        return false;

    if (m_tracy_gpu_context_ptr)
    {
        m_tracy_gpu_context_ptr->SetName(name);
    }
    return true;
}

Ptr<Rhi::ICommandKit> CommandQueue::CreateCommandKit()
{
    META_FUNCTION_TASK();
    return std::make_shared<CommandKit>(*this);
}

const Rhi::IContext& CommandQueue::GetContext() const noexcept
{
    META_FUNCTION_TASK();
    return dynamic_cast<const Rhi::IContext&>(m_context);
}

void CommandQueue::Execute(Rhi::ICommandListSet& command_lists, const Rhi::ICommandList::CompletedCallback& completed_callback)
{
    META_FUNCTION_TASK();
    META_LOG("Command queue '{}' is executing", GetName());
    static_cast<CommandListSet&>(command_lists).Execute(completed_callback);
}

Tracy::GpuContext& CommandQueue::GetTracyContext() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_tracy_gpu_context_ptr);
    return *m_tracy_gpu_context_ptr;
}

void CommandQueue::InitializeTracyGpuContext(const Tracy::GpuContext::Settings& tracy_settings)
{
    META_FUNCTION_TASK();
    m_tracy_gpu_context_ptr = std::make_unique<Tracy::GpuContext>(tracy_settings);
}

} // namespace Methane::Graphics::Base
