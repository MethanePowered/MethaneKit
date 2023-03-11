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

FILE: Methane/Graphics/RHI/ComputeCommandList.cpp
Methane ComputeCommandList PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#include <Methane/Graphics/RHI/ComputeCommandList.h>
#include <Methane/Graphics/RHI/ComputeState.h>
#include <Methane/Graphics/RHI/CommandListDebugGroup.h>
#include <Methane/Graphics/RHI/CommandQueue.h>

#include <Methane/Pimpl.hpp>

#ifdef META_GFX_METAL
#include <ComputeCommandList.hh>
#else
#include <ComputeCommandList.h>
#endif

namespace Methane::Graphics::Rhi
{

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(ComputeCommandList);
META_PIMPL_METHODS_COMPARE_IMPLEMENT(ComputeCommandList);

ComputeCommandList::ComputeCommandList(const Ptr<IComputeCommandList>& interface_ptr)
    : m_impl_ptr(std::dynamic_pointer_cast<Impl>(interface_ptr))
{
}

ComputeCommandList::ComputeCommandList(IComputeCommandList& interface_ref)
    : ComputeCommandList(interface_ref.GetDerivedPtr<IComputeCommandList>())
{
}

ComputeCommandList::ComputeCommandList(const CommandQueue& command_queue)
    : ComputeCommandList(IComputeCommandList::Create(command_queue.GetInterface()))
{
}

bool ComputeCommandList::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

IComputeCommandList& ComputeCommandList::GetInterface() const META_PIMPL_NOEXCEPT
{
    return *m_impl_ptr;
}

Ptr<IComputeCommandList> ComputeCommandList::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return m_impl_ptr;
}

bool ComputeCommandList::SetName(std::string_view name) const
{
    return GetImpl(m_impl_ptr).SetName(name);
}

std::string_view ComputeCommandList::GetName() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetName();
}

void ComputeCommandList::Connect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Connect(receiver);
}

void ComputeCommandList::Disconnect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Disconnect(receiver);
}

void ComputeCommandList::PushDebugGroup(const DebugGroup& debug_group) const
{
    GetImpl(m_impl_ptr).PushDebugGroup(debug_group.GetInterface());
}

void ComputeCommandList::PopDebugGroup() const
{
    GetImpl(m_impl_ptr).PopDebugGroup();
}

void ComputeCommandList::Reset(const DebugGroup* debug_group_ptr) const
{
    GetImpl(m_impl_ptr).Reset(debug_group_ptr ? debug_group_ptr->GetInterfacePtr().get() : nullptr);
}

void ComputeCommandList::ResetOnce(const DebugGroup* debug_group_ptr) const
{
    GetImpl(m_impl_ptr).ResetOnce(debug_group_ptr ? debug_group_ptr->GetInterfacePtr().get() : nullptr);
}

void ComputeCommandList::SetResourceBarriers(const IResourceBarriers& resource_barriers) const
{
    GetImpl(m_impl_ptr).SetResourceBarriers(resource_barriers);
}

void ComputeCommandList::Commit() const
{
    GetImpl(m_impl_ptr).Commit();
}

void ComputeCommandList::WaitUntilCompleted(uint32_t timeout_ms) const
{
    GetImpl(m_impl_ptr).WaitUntilCompleted(timeout_ms);
}

Data::TimeRange ComputeCommandList::GetGpuTimeRange(bool in_cpu_nanoseconds) const
{
    return GetImpl(m_impl_ptr).GetGpuTimeRange(in_cpu_nanoseconds);
}

CommandListState ComputeCommandList::GetState() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetState();
}

CommandQueue ComputeCommandList::GetCommandQueue() const
{
    return CommandQueue(GetImpl(m_impl_ptr).GetCommandQueue());
}

void ComputeCommandList::Connect(Data::Receiver<ICommandListCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<ICommandListCallback>::Connect(receiver);
}

void ComputeCommandList::Disconnect(Data::Receiver<ICommandListCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<ICommandListCallback>::Disconnect(receiver);
}

void ComputeCommandList::ResetWithState(ComputeState& compute_state, const DebugGroup* debug_group_ptr) const
{
    GetImpl(m_impl_ptr).ResetWithState(compute_state.GetInterface(),
                                       debug_group_ptr ? debug_group_ptr->GetInterfacePtr().get() : nullptr);
}

void ComputeCommandList::ResetWithStateOnce(ComputeState& compute_state, const DebugGroup* debug_group_ptr) const
{
    GetImpl(m_impl_ptr).ResetWithStateOnce(compute_state.GetInterface(),
                                           debug_group_ptr ? debug_group_ptr->GetInterfacePtr().get() : nullptr);
}

void ComputeCommandList::SetComputeState(ComputeState& compute_state) const
{
    GetImpl(m_impl_ptr).SetComputeState(compute_state.GetInterface());
}

void ComputeCommandList::Dispatch(const ThreadGroupsCount& thread_groups_count) const
{
    GetImpl(m_impl_ptr).Dispatch(thread_groups_count);
}

} // namespace Methane::Graphics::Rhi
