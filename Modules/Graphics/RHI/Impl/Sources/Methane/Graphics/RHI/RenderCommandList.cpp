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

FILE: Methane/Graphics/RHI/RenderCommandList.cpp
Methane RenderCommandList PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#include <Methane/Graphics/RHI/RenderCommandList.h>
#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/CommandListDebugGroup.h>
#include <Methane/Graphics/RHI/ResourceBarriers.h>
#include <Methane/Graphics/RHI/Buffer.h>
#include <Methane/Graphics/RHI/BufferSet.h>
#include <Methane/Graphics/RHI/RenderPass.h>
#include <Methane/Graphics/RHI/RenderState.h>
#include <Methane/Graphics/RHI/ViewState.h>
#include <Methane/Graphics/RHI/ProgramBindings.h>

#include <Methane/Pimpl.hpp>

#ifdef META_GFX_METAL
#include <RenderCommandList.hh>
#else
#include <RenderCommandList.h>
#endif

namespace Methane::Graphics::Rhi
{

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(RenderCommandList);

RenderCommandList::RenderCommandList(const Ptr<IRenderCommandList>& interface_ptr)
    : m_impl_ptr(std::dynamic_pointer_cast<Impl>(interface_ptr))
{
}

RenderCommandList::RenderCommandList(IRenderCommandList& interface_ref)
    : RenderCommandList(interface_ref.GetDerivedPtr<IRenderCommandList>())
{
}

RenderCommandList::RenderCommandList(const CommandQueue& command_queue, const RenderPass& render_pass)
    : RenderCommandList(IRenderCommandList::Create(command_queue.GetInterface(), render_pass.GetInterface()))
{
}

bool RenderCommandList::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

IRenderCommandList& RenderCommandList::GetInterface() const META_PIMPL_NOEXCEPT
{
    return *m_impl_ptr;
}

Ptr<IRenderCommandList> RenderCommandList::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return m_impl_ptr;
}

bool RenderCommandList::SetName(std::string_view name) const
{
    return GetImpl(m_impl_ptr).SetName(name);
}

std::string_view RenderCommandList::GetName() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetName();
}

void RenderCommandList::Connect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Connect(receiver);
}

void RenderCommandList::Disconnect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Disconnect(receiver);
}

void RenderCommandList::PushDebugGroup(const DebugGroup& debug_group) const
{
    GetImpl(m_impl_ptr).PushDebugGroup(debug_group.GetInterface());
}

void RenderCommandList::PopDebugGroup() const
{
    GetImpl(m_impl_ptr).PopDebugGroup();
}

void RenderCommandList::Reset(const DebugGroup* debug_group_ptr) const
{
    GetImpl(m_impl_ptr).Reset(debug_group_ptr ? debug_group_ptr->GetInterfacePtr().get() : nullptr);
}

void RenderCommandList::ResetOnce(const DebugGroup* debug_group_ptr) const
{
    GetImpl(m_impl_ptr).ResetOnce(debug_group_ptr ? debug_group_ptr->GetInterfacePtr().get() : nullptr);
}

void RenderCommandList::SetProgramBindings(const ProgramBindings& program_bindings, ProgramBindingsApplyBehaviorMask apply_behavior) const
{
    GetImpl(m_impl_ptr).SetProgramBindings(program_bindings.GetInterface(), apply_behavior);
}

void RenderCommandList::SetResourceBarriers(const ResourceBarriers& resource_barriers) const
{
    GetImpl(m_impl_ptr).SetResourceBarriers(resource_barriers.GetInterface());
}

void RenderCommandList::Commit() const
{
    GetImpl(m_impl_ptr).Commit();
}

void RenderCommandList::WaitUntilCompleted(uint32_t timeout_ms) const
{
    GetImpl(m_impl_ptr).WaitUntilCompleted(timeout_ms);
}

Data::TimeRange RenderCommandList::GetGpuTimeRange(bool in_cpu_nanoseconds) const
{
    return GetImpl(m_impl_ptr).GetGpuTimeRange(in_cpu_nanoseconds);
}

CommandListState RenderCommandList::GetState() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetState();
}

CommandQueue RenderCommandList::GetCommandQueue() const
{
    return CommandQueue(GetImpl(m_impl_ptr).GetCommandQueue());
}

void RenderCommandList::Connect(Data::Receiver<ICommandListCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<ICommandListCallback>::Connect(receiver);
}

void RenderCommandList::Disconnect(Data::Receiver<ICommandListCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<ICommandListCallback>::Disconnect(receiver);
}

bool RenderCommandList::IsValidationEnabled() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).IsValidationEnabled();
}

void RenderCommandList::SetValidationEnabled(bool is_validation_enabled) const
{
    GetImpl(m_impl_ptr).SetValidationEnabled(is_validation_enabled);
}

RenderPass RenderCommandList::GetRenderPass() const
{
    return RenderPass(GetImpl(m_impl_ptr).GetRenderPass());
}

void RenderCommandList::ResetWithState(const RenderState& render_state, const DebugGroup* debug_group_ptr) const
{
    GetImpl(m_impl_ptr).ResetWithState(render_state.GetInterface(), debug_group_ptr ? debug_group_ptr->GetInterfacePtr().get() : nullptr);
}

void RenderCommandList::ResetWithStateOnce(const RenderState& render_state, const DebugGroup* debug_group_ptr) const
{
    GetImpl(m_impl_ptr).ResetWithStateOnce(render_state.GetInterface(), debug_group_ptr ? debug_group_ptr->GetInterfacePtr().get() : nullptr);
}

void RenderCommandList::SetRenderState(const RenderState& render_state, RenderStateGroupMask state_groups) const
{
    GetImpl(m_impl_ptr).SetRenderState(render_state.GetInterface(), state_groups);
}

void RenderCommandList::SetViewState(const ViewState& view_state) const
{
    GetImpl(m_impl_ptr).SetViewState(view_state.GetInterface());
}

bool RenderCommandList::SetVertexBuffers(const BufferSet& vertex_buffers, bool set_resource_barriers) const
{
    return GetImpl(m_impl_ptr).SetVertexBuffers(vertex_buffers.GetInterface(), set_resource_barriers);
}

bool RenderCommandList::SetIndexBuffer(const Buffer& index_buffer, bool set_resource_barriers) const
{
    return GetImpl(m_impl_ptr).SetIndexBuffer(index_buffer.GetInterface(), set_resource_barriers);
}

void RenderCommandList::DrawIndexed(Primitive primitive, uint32_t index_count,
                                    uint32_t start_index, uint32_t start_vertex,
                                    uint32_t instance_count, uint32_t start_instance) const
{
    GetImpl(m_impl_ptr).DrawIndexed(primitive, index_count, start_index, start_vertex, instance_count, start_instance);
}

void RenderCommandList::Draw(Primitive primitive,
                             uint32_t vertex_count, uint32_t start_vertex,
                             uint32_t instance_count, uint32_t start_instance) const
{
    GetImpl(m_impl_ptr).Draw(primitive, vertex_count, start_vertex, instance_count, start_instance);
}

} // namespace Methane::Graphics::Rhi
