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

FILE: Methane/Graphics/RHI/ParallelRenderCommandList.cpp
Methane ParallelRenderCommandList PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#include <Methane/Graphics/RHI/ParallelRenderCommandList.h>
#include <Methane/Graphics/RHI/RenderPass.h>
#include <Methane/Graphics/RHI/RenderState.h>
#include <Methane/Graphics/RHI/ViewState.h>
#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/CommandListDebugGroup.h>
#include <Methane/Graphics/RHI/RenderCommandList.h>
#include <Methane/Graphics/RHI/ResourceBarriers.h>

#include <Methane/Pimpl.hpp>

#ifdef META_GFX_METAL
#include <ParallelRenderCommandList.hh>
#else
#include <ParallelRenderCommandList.h>
#endif

namespace Methane::Graphics::Rhi
{

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(ParallelRenderCommandList);
META_PIMPL_METHODS_COMPARE_IMPLEMENT(ParallelRenderCommandList);

ParallelRenderCommandList::ParallelRenderCommandList(const Ptr<IParallelRenderCommandList>& interface_ptr)
    : m_impl_ptr(std::dynamic_pointer_cast<Impl>(interface_ptr))
{
}

ParallelRenderCommandList::ParallelRenderCommandList(IParallelRenderCommandList& interface_ref)
    : ParallelRenderCommandList(interface_ref.GetDerivedPtr<IParallelRenderCommandList>())
{
}

ParallelRenderCommandList::ParallelRenderCommandList(const CommandQueue& command_queue, const RenderPass& render_pass)
    : ParallelRenderCommandList(IParallelRenderCommandList::Create(command_queue.GetInterface(), render_pass.GetInterface()))
{
}

bool ParallelRenderCommandList::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

IParallelRenderCommandList& ParallelRenderCommandList::GetInterface() const META_PIMPL_NOEXCEPT
{
    return *m_impl_ptr;
}

Ptr<IParallelRenderCommandList> ParallelRenderCommandList::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return m_impl_ptr;
}

bool ParallelRenderCommandList::SetName(std::string_view name) const
{
    return GetImpl(m_impl_ptr).SetName(name);
}

std::string_view ParallelRenderCommandList::GetName() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetName();
}

void ParallelRenderCommandList::Connect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Connect(receiver);
}

void ParallelRenderCommandList::Disconnect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Disconnect(receiver);
}

void ParallelRenderCommandList::PushDebugGroup(const DebugGroup& debug_group) const
{
    GetImpl(m_impl_ptr).PushDebugGroup(debug_group.GetInterface());
}

void ParallelRenderCommandList::PopDebugGroup() const
{
    GetImpl(m_impl_ptr).PopDebugGroup();
}

void ParallelRenderCommandList::Reset(const DebugGroup* debug_group_ptr) const
{
    GetImpl(m_impl_ptr).Reset(debug_group_ptr ? &debug_group_ptr->GetInterface() : nullptr);
}

void ParallelRenderCommandList::ResetOnce(const DebugGroup* debug_group_ptr) const
{
    GetImpl(m_impl_ptr).ResetOnce(debug_group_ptr ? &debug_group_ptr->GetInterface() : nullptr);
}

void ParallelRenderCommandList::SetProgramBindings(IProgramBindings& program_bindings, ProgramBindingsApplyBehaviorMask apply_behavior) const
{
    GetImpl(m_impl_ptr).SetProgramBindings(program_bindings, apply_behavior);
}

void ParallelRenderCommandList::SetResourceBarriers(const ResourceBarriers& resource_barriers) const
{
    GetImpl(m_impl_ptr).SetResourceBarriers(resource_barriers.GetInterface());
}

void ParallelRenderCommandList::Commit() const
{
    GetImpl(m_impl_ptr).Commit();
}

void ParallelRenderCommandList::WaitUntilCompleted(uint32_t timeout_ms) const
{
    GetImpl(m_impl_ptr).WaitUntilCompleted(timeout_ms);
}

Data::TimeRange ParallelRenderCommandList::GetGpuTimeRange(bool in_cpu_nanoseconds) const
{
    return GetImpl(m_impl_ptr).GetGpuTimeRange(in_cpu_nanoseconds);
}

CommandListState ParallelRenderCommandList::GetState() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetState();
}

CommandQueue ParallelRenderCommandList::GetCommandQueue() const
{
    return CommandQueue(GetImpl(m_impl_ptr).GetCommandQueue());
}

void ParallelRenderCommandList::Connect(Data::Receiver<ICommandListCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<ICommandListCallback>::Connect(receiver);
}

void ParallelRenderCommandList::Disconnect(Data::Receiver<ICommandListCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<ICommandListCallback>::Disconnect(receiver);
}

bool ParallelRenderCommandList::IsValidationEnabled() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).IsValidationEnabled();
}

void ParallelRenderCommandList::SetValidationEnabled(bool is_validation_enabled) const
{
    GetImpl(m_impl_ptr).SetValidationEnabled(is_validation_enabled);
}

void ParallelRenderCommandList::ResetWithState(const RenderState& render_state, const DebugGroup* debug_group_ptr) const
{
    GetImpl(m_impl_ptr).ResetWithState(render_state.GetInterface(), debug_group_ptr ? &debug_group_ptr->GetInterface() : nullptr);
}

void ParallelRenderCommandList::SetViewState(const ViewState& view_state) const
{
    GetImpl(m_impl_ptr).SetViewState(view_state.GetInterface());
}

void ParallelRenderCommandList::SetBeginningResourceBarriers(const ResourceBarriers& resource_barriers) const
{
    GetImpl(m_impl_ptr).SetBeginningResourceBarriers(resource_barriers.GetInterface());
}

void ParallelRenderCommandList::SetEndingResourceBarriers(const ResourceBarriers& resource_barriers) const
{
    GetImpl(m_impl_ptr).SetEndingResourceBarriers(resource_barriers.GetInterface());
}

void ParallelRenderCommandList::SetParallelCommandListsCount(uint32_t count) const
{
    GetImpl(m_impl_ptr).SetParallelCommandListsCount(count);
}

const std::vector<RenderCommandList>& ParallelRenderCommandList::GetParallelCommandLists() const
{
    if (!m_parallel_command_lists.empty())
        return m_parallel_command_lists;

    const Refs<Rhi::IRenderCommandList>& command_list_refs = GetImpl(m_impl_ptr).GetParallelCommandLists();
    for(const Ref<Rhi::IRenderCommandList> command_list_ref : command_list_refs)
    {
        m_parallel_command_lists.emplace_back(command_list_ref.get());
    }

    return m_parallel_command_lists;
}

} // namespace Methane::Graphics::Rhi
