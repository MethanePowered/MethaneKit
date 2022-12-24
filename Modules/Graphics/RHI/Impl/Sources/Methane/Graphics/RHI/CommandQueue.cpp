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
#include <Methane/Graphics/RHI/CommandListSet.h>

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

class CommandQueue::Impl
    : public ImplWrapper<ICommandQueue, CommandQueueImpl>
{
public:
    using ImplWrapper::ImplWrapper;
};

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(CommandQueue);

CommandQueue::CommandQueue(ImplPtr<Impl>&& impl_ptr)
    : Data::Transmitter<IObjectCallback>(impl_ptr->GetInterface())
    , m_impl_ptr(std::move(impl_ptr))
{
}

CommandQueue::CommandQueue(const Ptr<ICommandQueue>& interface_ptr)
    : CommandQueue(std::make_unique<Impl>(interface_ptr))
{
}

CommandQueue::CommandQueue(ICommandQueue& interface_ref)
    : CommandQueue(interface_ref.GetDerivedPtr<ICommandQueue>())
{
}

CommandQueue::CommandQueue(const RenderContext& context, CommandListType command_lists_type)
    : CommandQueue(ICommandQueue::Create(context.GetInterface(), command_lists_type))
{
}

void CommandQueue::Init(const RenderContext& context, CommandListType command_lists_type)
{
    m_impl_ptr = std::make_unique<Impl>(ICommandQueue::Create(context.GetInterface(), command_lists_type));
    Transmitter::Reset(&m_impl_ptr->GetInterface());
}

void CommandQueue::Release()
{
    Transmitter::Reset();
    m_impl_ptr.reset();
}

bool CommandQueue::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

ICommandQueue& CommandQueue::GetInterface() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterface(m_impl_ptr);
}

Ptr<ICommandQueue> CommandQueue::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterfacePtr(m_impl_ptr);
}

bool CommandQueue::SetName(std::string_view name) const
{
    return GetPrivateImpl(m_impl_ptr).SetName(name);
}

std::string_view CommandQueue::GetName() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetName();
}

[[nodiscard]] const IContext& CommandQueue::GetContext() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetContext();
}

[[nodiscard]] CommandListType CommandQueue::GetCommandListType() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetCommandListType();
}

[[nodiscard]] uint32_t CommandQueue::GetFamilyIndex() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetFamilyIndex();
}

[[nodiscard]] ITimestampQueryPool* CommandQueue::GetTimestampQueryPool() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetTimestampQueryPool();
}

void CommandQueue::Execute(const CommandListSet& command_lists, const ICommandList::CompletedCallback& completed_callback) const
{
    return GetPrivateImpl(m_impl_ptr).Execute(command_lists.GetInterface(), completed_callback);
}

} // namespace Methane::Graphics::Rhi
