/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/CommandQueue.cpp
Methane CommandQueue PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/RenderContext.h>

#if defined METHANE_GFX_DIRECTX

#include <Methane/Graphics/DirectX/CommandQueue.h>
using CommandQueueImpl = Methane::Graphics::DirectX::CommandQueue;

#elif defined METHANE_GFX_VULKAN

#include <Methane/Graphics/Vulkan/CommandQueue.h>
using CommandQueueImpl = Methane::Graphics::Vulkan::CommandQueue;

#elif defined METHANE_GFX_METAL

#include <Methane/Graphics/Metal/CommandQueue.hh>
using CommandQueueImpl = Methane::Graphics::Metal::CommandQueue;

#else // METHAN_GFX_[API] is undefined

static_assert(false, "Static graphics API macro-definition is missing.");

#endif

#include "ImplWrapper.hpp"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

class CommandQueue::Impl : public ImplWrapper<ICommandQueue, CommandQueueImpl>
{
public:
    using ImplWrapper::ImplWrapper;
};

CommandQueue::CommandQueue(const Ptr<ICommandQueue>& interface_ptr)
    : m_impl_ptr(std::make_unique<Impl>(interface_ptr))
{
}

CommandQueue::CommandQueue(const RenderContext& context, CommandListType command_lists_type)
    : CommandQueue(ICommandQueue::Create(context.GetInterface(), command_lists_type))
{
}

void CommandQueue::Init(const RenderContext& context, CommandListType command_lists_type)
{
    m_impl_ptr = std::make_unique<Impl>(ICommandQueue::Create(context.GetInterface(), command_lists_type));
}

void CommandQueue::Release()
{
    m_impl_ptr.release();
}

bool CommandQueue::IsInitialized() const noexcept
{
    return static_cast<bool>(m_impl_ptr);
}

ICommandQueue& CommandQueue::GetInterface() const noexcept
{
    return m_impl_ptr->GetInterface();
}

bool CommandQueue::SetName(const std::string& name) const
{
    return m_impl_ptr->Get().SetName(name);
}

const std::string& CommandQueue::GetName() const noexcept
{
    return m_impl_ptr->Get().GetName();
}

} // namespace Methane::Graphics::Rhi
