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

FILE: Methane/Graphics/RHI/CommandKit.cpp
Methane CommandKit PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#include <Methane/Graphics/RHI/CommandKit.h>
#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/RenderContext.h>
#include <Methane/Graphics/RHI/RenderCommandList.h>
#include <Methane/Graphics/RHI/ComputeCommandList.h>
#include <Methane/Graphics/RHI/CommandListSet.h>

#include <Methane/Graphics/Base/CommandKit.h>

#include <Methane/Pimpl.hpp>

namespace Methane::Graphics::Rhi
{

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(CommandKit);
META_PIMPL_METHODS_COMPARE_IMPLEMENT(CommandKit);

CommandKit::CommandKit(const Ptr<ICommandKit>& interface_ptr)
    : m_impl_ptr(std::dynamic_pointer_cast<Impl>(interface_ptr))
{
}

CommandKit::CommandKit(ICommandKit& interface_ref)
    : CommandKit(interface_ref.GetDerivedPtr<ICommandKit>())
{
}

CommandKit::CommandKit(const CommandQueue& command_queue)
    : CommandKit(ICommandKit::Create(command_queue.GetInterface()))
{
}

CommandKit::CommandKit(const RenderContext& context, CommandListType command_lists_type)
    : CommandKit(ICommandKit::Create(context.GetInterface(), command_lists_type))
{
}

bool CommandKit::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

ICommandKit& CommandKit::GetInterface() const META_PIMPL_NOEXCEPT
{
    return *m_impl_ptr;
}

Ptr<ICommandKit> CommandKit::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return m_impl_ptr;
}

bool CommandKit::SetName(std::string_view name) const
{
    return GetImpl(m_impl_ptr).SetName(name);
}

std::string_view CommandKit::GetName() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetName();
}

void CommandKit::Connect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Connect(receiver);
}

void CommandKit::Disconnect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Disconnect(receiver);
}

const IContext& CommandKit::GetContext() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetContext();
}

CommandQueue CommandKit::GetQueue() const
{
    return CommandQueue(GetImpl(m_impl_ptr).GetQueue());
}

CommandListType CommandKit::GetListType() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetListType();
}

bool CommandKit::HasList(CommandListId cmd_list_id) const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).HasList(cmd_list_id);
}

bool CommandKit::HasListWithState(CommandListState cmd_list_state, CommandListId cmd_list_id) const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).HasListWithState(cmd_list_state, cmd_list_id);
}

RenderCommandList CommandKit::GetRenderList(CommandListId cmd_list_id) const
{
    META_CHECK_ARG_EQUAL(GetListType(), CommandListType::Render);
    return RenderCommandList(dynamic_cast<IRenderCommandList&>(GetImpl(m_impl_ptr).GetList(cmd_list_id)));
}

RenderCommandList CommandKit::GetRenderListForEncoding(CommandListId cmd_list_id, std::string_view debug_group_name) const
{
    META_CHECK_ARG_EQUAL(GetListType(), CommandListType::Render);
    return RenderCommandList(dynamic_cast<IRenderCommandList&>(GetImpl(m_impl_ptr).GetListForEncoding(cmd_list_id, debug_group_name)));
}

ComputeCommandList CommandKit::GetComputeList(CommandListId cmd_list_id) const
{
    META_CHECK_ARG_EQUAL(GetListType(), CommandListType::Compute);
    return ComputeCommandList(dynamic_cast<IComputeCommandList&>(GetImpl(m_impl_ptr).GetList(cmd_list_id)));
}

ComputeCommandList CommandKit::GetComputeListForEncoding(CommandListId cmd_list_id, std::string_view debug_group_name) const
{
    META_CHECK_ARG_EQUAL(GetListType(), CommandListType::Compute);
    return ComputeCommandList(dynamic_cast<IComputeCommandList&>(GetImpl(m_impl_ptr).GetListForEncoding(cmd_list_id, debug_group_name)));
}

CommandListSet CommandKit::GetListSet(const std::vector<CommandListId>& cmd_list_ids, Opt<Data::Index> frame_index_opt) const
{
    return CommandListSet(GetImpl(m_impl_ptr).GetListSet(cmd_list_ids, frame_index_opt));
}

IFence& CommandKit::GetFence(CommandListId fence_id) const
{
    return GetImpl(m_impl_ptr).GetFence(fence_id);
}

void CommandKit::ExecuteListSet(const std::vector<Rhi::CommandListId>& cmd_list_ids, Opt<Data::Index> frame_index_opt) const
{
    GetImpl(m_impl_ptr).ExecuteListSet(cmd_list_ids, frame_index_opt);
}

void CommandKit::ExecuteListSetAndWaitForCompletion(const std::vector<Rhi::CommandListId>& cmd_list_ids, Opt<Data::Index> frame_index_opt) const
{
    GetImpl(m_impl_ptr).ExecuteListSetAndWaitForCompletion(cmd_list_ids, frame_index_opt);
}

} // namespace Methane::Graphics::Rhi
