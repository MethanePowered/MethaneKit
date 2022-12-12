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

FILE: Methane/Graphics/Metal/CommandQueue.mm
Metal implementation of the command queue interface.

******************************************************************************/

#include <Methane/Graphics/Metal/CommandQueue.hh>
#include <Methane/Graphics/Metal/Device.hh>
#include <Methane/Graphics/Metal/RenderContext.hh>

#include <Methane/Platform/Apple/Types.hh>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <QuartzCore/CABase.h>

namespace Methane::Graphics::Rhi
{

Ptr<ICommandQueue> ICommandQueue::Create(const IContext& context, CommandListType command_lists_type)
{
    META_FUNCTION_TASK();
    return std::make_shared<Metal::CommandQueue>(dynamic_cast<const Base::Context&>(context), command_lists_type);
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::Metal
{

CommandQueue::CommandQueue(const Base::Context& context, Rhi::CommandListType command_lists_type)
    : Base::CommandQueue(context, command_lists_type)
    , m_mtl_command_queue([GetMetalContext().GetMetalDevice().GetNativeDevice() newCommandQueue])
{
    META_FUNCTION_TASK();
    InitializeTracyGpuContext(
        Tracy::GpuContext::Settings(
            Tracy::GpuContext::Type::Metal,
            0U,
            Data::ConvertTimeSecondsToNanoseconds(CACurrentMediaTime())
        )
    );
}

bool CommandQueue::SetName(std::string_view name)
{
    META_FUNCTION_TASK();
    if (!Base::CommandQueue::SetName(name))
        return false;

    META_CHECK_ARG_NOT_NULL(m_mtl_command_queue);
    m_mtl_command_queue.label = MacOS::ConvertToNsString(name);
    return true;
}

const IContext& CommandQueue::GetMetalContext() const noexcept
{
    META_FUNCTION_TASK();
    return dynamic_cast<const IContext&>(GetBaseContext());
}

const RenderContext& CommandQueue::GetMetalRenderContext() const
{
    META_FUNCTION_TASK();
    const Base::Context& context = GetBaseContext();
    META_CHECK_ARG_EQUAL_DESCR(context.GetType(), Rhi::ContextType::Render, "incompatible context type");
    return static_cast<const RenderContext&>(context);
}

} // namespace Methane::Graphics::Metal
