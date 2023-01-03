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

#include "Pimpl.hpp"

#ifdef META_GFX_METAL
#include <CommandQueue.hh>
#else
#include <CommandQueue.h>
#endif

namespace Methane::Graphics::Rhi
{

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(CommandQueue);
META_PIMPL_METHODS_COMPARE_IMPLEMENT(CommandQueue);

CommandQueue::CommandQueue(Ptr<Impl>&& impl_ptr)
    : m_impl_ptr(std::move(impl_ptr))
{
}

CommandQueue::CommandQueue(const Ptr<ICommandQueue>& interface_ptr)
    : CommandQueue(std::dynamic_pointer_cast<Impl>(interface_ptr))
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
    m_impl_ptr = std::dynamic_pointer_cast<Impl>(ICommandQueue::Create(context.GetInterface(), command_lists_type));
}

void CommandQueue::Release()
{
    m_impl_ptr.reset();
}

bool CommandQueue::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

ICommandQueue& CommandQueue::GetInterface() const META_PIMPL_NOEXCEPT
{
    return *m_impl_ptr;
}

Ptr<ICommandQueue> CommandQueue::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return m_impl_ptr;
}

bool CommandQueue::SetName(std::string_view name) const
{
    return GetImpl(m_impl_ptr).SetName(name);
}

std::string_view CommandQueue::GetName() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetName();
}

void CommandQueue::Connect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Connect(receiver);
}

void CommandQueue::Disconnect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Disconnect(receiver);
}

[[nodiscard]] const IContext& CommandQueue::GetContext() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetContext();
}

[[nodiscard]] CommandListType CommandQueue::GetCommandListType() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetCommandListType();
}

[[nodiscard]] uint32_t CommandQueue::GetFamilyIndex() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetFamilyIndex();
}

[[nodiscard]] ITimestampQueryPool* CommandQueue::GetTimestampQueryPool() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetTimestampQueryPool();
}

void CommandQueue::Execute(const CommandListSet& command_lists, const ICommandList::CompletedCallback& completed_callback) const
{
    return GetImpl(m_impl_ptr).Execute(command_lists.GetInterface(), completed_callback);
}

} // namespace Methane::Graphics::Rhi