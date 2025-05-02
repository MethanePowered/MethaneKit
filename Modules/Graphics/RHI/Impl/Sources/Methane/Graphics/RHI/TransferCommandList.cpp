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

FILE: Methane/Graphics/RHI/TransferCommandList.cpp
Methane TransferCommandList PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#include <Methane/Graphics/RHI/TransferCommandList.h>
#include <Methane/Graphics/RHI/CommandListDebugGroup.h>
#include <Methane/Graphics/RHI/CommandQueue.h>

#include <Methane/Pimpl.hpp>

#ifdef META_GFX_METAL
#include <TransferCommandList.hh>
#else
#include <TransferCommandList.h>
#endif

namespace Methane::Graphics::Rhi
{

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(TransferCommandList);

TransferCommandList::TransferCommandList(const Ptr<ITransferCommandList>& interface_ptr)
    : m_impl_ptr(std::dynamic_pointer_cast<Impl>(interface_ptr))
{
}

TransferCommandList::TransferCommandList(ITransferCommandList& interface_ref)
    : TransferCommandList(interface_ref.GetDerivedPtr<ITransferCommandList>())
{
}

TransferCommandList::TransferCommandList(const CommandQueue& command_queue)
    : TransferCommandList(ITransferCommandList::Create(command_queue.GetInterface()))
{
}

bool TransferCommandList::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

ITransferCommandList& TransferCommandList::GetInterface() const META_PIMPL_NOEXCEPT
{
    return *m_impl_ptr;
}

Ptr<ITransferCommandList> TransferCommandList::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return m_impl_ptr;
}

bool TransferCommandList::SetName(std::string_view name) const
{
    return GetImpl(m_impl_ptr).SetName(name);
}

std::string_view TransferCommandList::GetName() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetName();
}

void TransferCommandList::Connect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Connect(receiver);
}

void TransferCommandList::Disconnect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Disconnect(receiver);
}

void TransferCommandList::PushDebugGroup(const DebugGroup& debug_group) const
{
    GetImpl(m_impl_ptr).PushDebugGroup(debug_group.GetInterface());
}

void TransferCommandList::PopDebugGroup() const
{
    GetImpl(m_impl_ptr).PopDebugGroup();
}

void TransferCommandList::Reset(const DebugGroup* debug_group_ptr) const
{
    GetImpl(m_impl_ptr).Reset(debug_group_ptr ? debug_group_ptr->GetInterfacePtr().get() : nullptr);
}

void TransferCommandList::ResetOnce(const DebugGroup* debug_group_ptr) const
{
    GetImpl(m_impl_ptr).ResetOnce(debug_group_ptr ? debug_group_ptr->GetInterfacePtr().get() : nullptr);
}

void TransferCommandList::SetResourceBarriers(const IResourceBarriers& resource_barriers) const
{
    GetImpl(m_impl_ptr).SetResourceBarriers(resource_barriers);
}

void TransferCommandList::Commit() const
{
    GetImpl(m_impl_ptr).Commit();
}

void TransferCommandList::WaitUntilCompleted(uint32_t timeout_ms) const
{
    GetImpl(m_impl_ptr).WaitUntilCompleted(timeout_ms);
}

Data::TimeRange TransferCommandList::GetGpuTimeRange(bool in_cpu_nanoseconds) const
{
    return GetImpl(m_impl_ptr).GetGpuTimeRange(in_cpu_nanoseconds);
}

CommandListState TransferCommandList::GetState() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetState();
}

CommandQueue TransferCommandList::GetCommandQueue() const
{
    return CommandQueue(GetImpl(m_impl_ptr).GetCommandQueue());
}

void TransferCommandList::Connect(Data::Receiver<ICommandListCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<ICommandListCallback>::Connect(receiver);
}

void TransferCommandList::Disconnect(Data::Receiver<ICommandListCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<ICommandListCallback>::Disconnect(receiver);
}


} // namespace Methane::Graphics::Rhi
