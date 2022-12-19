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

#if defined METHANE_GFX_DIRECTX

#include <Methane/Graphics/DirectX/ParallelRenderCommandList.h>
using ParallelRenderCommandListImpl = Methane::Graphics::DirectX::ParallelRenderCommandList;

#elif defined METHANE_GFX_VULKAN

#include <Methane/Graphics/Vulkan/ParallelRenderCommandList.h>
using ParallelRenderCommandListImpl = Methane::Graphics::Vulkan::ParallelRenderCommandList;

#elif defined METHANE_GFX_METAL

#include <Methane/Graphics/Metal/ParallelRenderCommandList.hh>
using ParallelRenderCommandListImpl = Methane::Graphics::Metal::ParallelRenderCommandList;

#else // METHAN_GFX_[API] is undefined

static_assert(false, "Static graphics API macro-definition is missing.");

#endif

#include "ImplWrapper.hpp"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

class ParallelRenderCommandList::Impl
    : public ImplWrapper<IParallelRenderCommandList, ParallelRenderCommandListImpl>
{
public:
    using ImplWrapper::ImplWrapper;
};

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(ParallelRenderCommandList);

ParallelRenderCommandList::ParallelRenderCommandList(UniquePtr<Impl>&& impl_ptr)
    : Transmitter<Rhi::ICommandListCallback>(impl_ptr->GetInterface())
    , Transmitter<Rhi::IObjectCallback>(impl_ptr->GetInterface())
    , m_impl_ptr(std::move(impl_ptr))
{
}

ParallelRenderCommandList::ParallelRenderCommandList(const Ptr<IParallelRenderCommandList>& interface_ptr)
    : ParallelRenderCommandList(std::make_unique<Impl>(interface_ptr))
{
}

ParallelRenderCommandList::ParallelRenderCommandList(IParallelRenderCommandList& interface_ref)
    : ParallelRenderCommandList(std::dynamic_pointer_cast<IParallelRenderCommandList>(interface_ref.GetPtr()))
{
}

ParallelRenderCommandList::ParallelRenderCommandList(const CommandQueue& command_queue, const RenderPass& render_pass)
    : ParallelRenderCommandList(IParallelRenderCommandList::Create(command_queue.GetInterface(), render_pass.GetInterface()))
{
}

void ParallelRenderCommandList::Init(const CommandQueue& command_queue, const RenderPass& render_pass)
{
    m_impl_ptr = std::make_unique<Impl>(IParallelRenderCommandList::Create(command_queue.GetInterface(), render_pass.GetInterface()));
    Transmitter<Rhi::ICommandListCallback>::Reset(&m_impl_ptr->GetInterface());
    Transmitter<Rhi::IObjectCallback>::Reset(&m_impl_ptr->GetInterface());
}

void ParallelRenderCommandList::Release()
{
    Transmitter<Rhi::ICommandListCallback>::Reset();
    Transmitter<Rhi::IObjectCallback>::Reset();
    m_impl_ptr.release();
}

bool ParallelRenderCommandList::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

IParallelRenderCommandList& ParallelRenderCommandList::GetInterface() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterface(m_impl_ptr);
}

bool ParallelRenderCommandList::SetName(std::string_view name) const
{
    return GetPrivateImpl(m_impl_ptr).SetName(name);
}

std::string_view ParallelRenderCommandList::GetName() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetName();
}

void ParallelRenderCommandList::PushDebugGroup(DebugGroup& debug_group)
{
    GetPrivateImpl(m_impl_ptr).PushDebugGroup(debug_group.GetInterface());
}

void ParallelRenderCommandList::PopDebugGroup()
{
    GetPrivateImpl(m_impl_ptr).PopDebugGroup();
}

void ParallelRenderCommandList::Reset(DebugGroup* debug_group_ptr)
{
    GetPrivateImpl(m_impl_ptr).Reset(debug_group_ptr ? &debug_group_ptr->GetInterface() : nullptr);
}

void ParallelRenderCommandList::ResetOnce(DebugGroup* debug_group_ptr)
{
    GetPrivateImpl(m_impl_ptr).ResetOnce(debug_group_ptr ? &debug_group_ptr->GetInterface() : nullptr);
}

void ParallelRenderCommandList::SetProgramBindings(IProgramBindings& program_bindings, ProgramBindingsApplyBehaviorMask apply_behavior)
{
    GetPrivateImpl(m_impl_ptr).SetProgramBindings(program_bindings, apply_behavior);
}

void ParallelRenderCommandList::SetResourceBarriers(const ResourceBarriers& resource_barriers)
{
    GetPrivateImpl(m_impl_ptr).SetResourceBarriers(resource_barriers.GetInterface());
}

void ParallelRenderCommandList::Commit()
{
    GetPrivateImpl(m_impl_ptr).Commit();
}

void ParallelRenderCommandList::WaitUntilCompleted(uint32_t timeout_ms)
{
    GetPrivateImpl(m_impl_ptr).WaitUntilCompleted(timeout_ms);
}

Data::TimeRange ParallelRenderCommandList::GetGpuTimeRange(bool in_cpu_nanoseconds) const
{
    return GetPrivateImpl(m_impl_ptr).GetGpuTimeRange(in_cpu_nanoseconds);
}

CommandListState ParallelRenderCommandList::GetState() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetState();
}

CommandQueue ParallelRenderCommandList::GetCommandQueue()
{
    return CommandQueue(GetPrivateImpl(m_impl_ptr).GetCommandQueue());
}

bool ParallelRenderCommandList::IsValidationEnabled() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).IsValidationEnabled();
}

void ParallelRenderCommandList::SetValidationEnabled(bool is_validation_enabled) const
{
    GetPrivateImpl(m_impl_ptr).SetValidationEnabled(is_validation_enabled);
}

void ParallelRenderCommandList::ResetWithState(const RenderState& render_state, const DebugGroup* debug_group_ptr) const
{
    GetPrivateImpl(m_impl_ptr).ResetWithState(render_state.GetInterface(), debug_group_ptr ? &debug_group_ptr->GetInterface() : nullptr);
}

void ParallelRenderCommandList::SetViewState(const ViewState& view_state) const
{
    GetPrivateImpl(m_impl_ptr).SetViewState(view_state.GetInterface());
}

void ParallelRenderCommandList::SetBeginningResourceBarriers(const ResourceBarriers& resource_barriers) const
{
    GetPrivateImpl(m_impl_ptr).SetBeginningResourceBarriers(resource_barriers.GetInterface());
}

void ParallelRenderCommandList::SetEndingResourceBarriers(const ResourceBarriers& resource_barriers) const
{
    GetPrivateImpl(m_impl_ptr).SetEndingResourceBarriers(resource_barriers.GetInterface());
}

void ParallelRenderCommandList::SetParallelCommandListsCount(uint32_t count) const
{
    GetPrivateImpl(m_impl_ptr).SetParallelCommandListsCount(count);
}

const std::vector<RenderCommandList>& ParallelRenderCommandList::GetParallelCommandLists() const
{
    if (!m_parallel_command_lists.empty())
        return m_parallel_command_lists;

    const Refs<Rhi::IRenderCommandList>& command_list_refs = GetPrivateImpl(m_impl_ptr).GetParallelCommandLists();
    for(const Ref<Rhi::IRenderCommandList> command_list_ref : command_list_refs)
    {
        m_parallel_command_lists.emplace_back(command_list_ref.get());
    }

    return m_parallel_command_lists;
}

} // namespace Methane::Graphics::Rhi
