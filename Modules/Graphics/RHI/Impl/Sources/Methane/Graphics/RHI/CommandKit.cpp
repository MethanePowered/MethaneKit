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
#include <Methane/Graphics/RHI/CommandListSet.h>

#include <Methane/Graphics/Base/CommandKit.h>
using CommandKitImpl = Methane::Graphics::Base::CommandKit;

#include "ImplWrapper.hpp"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

class CommandKit::Impl
    : public ImplWrapper<ICommandKit, CommandKitImpl>
{
public:
    using ImplWrapper::ImplWrapper;
};

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(CommandKit);

CommandKit::CommandKit(const Ptr<ICommandKit>& interface_ptr)
    : m_impl_ptr(std::make_unique<Impl>(interface_ptr))
{
}

CommandKit::CommandKit(ICommandKit& interface)
    : CommandKit(std::dynamic_pointer_cast<ICommandKit>(interface.GetPtr()))
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

void CommandKit::Init(const CommandQueue& command_queue)
{
    m_impl_ptr = std::make_unique<Impl>(ICommandKit::Create(command_queue.GetInterface()));
}

void CommandKit::Init(const RenderContext& context, CommandListType command_lists_type)
{
    m_impl_ptr = std::make_unique<Impl>(ICommandKit::Create(context.GetInterface(), command_lists_type));
}

void CommandKit::Release()
{
    m_impl_ptr.release();
}

bool CommandKit::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

ICommandKit& CommandKit::GetInterface() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterface(m_impl_ptr);
}

bool CommandKit::SetName(const std::string& name) const
{
    return GetPrivateImpl(m_impl_ptr).SetName(name);
}

const std::string& CommandKit::GetName() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetName();
}

const IContext& CommandKit::GetContext() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetContext();
}

CommandQueue CommandKit::GetQueue() const
{
    return CommandQueue(GetPrivateImpl(m_impl_ptr).GetQueue());
}

CommandListType CommandKit::GetListType() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetListType();
}

bool CommandKit::HasList(CommandListId cmd_list_id) const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).HasList(cmd_list_id);
}

bool CommandKit::HasListWithState(CommandListState cmd_list_state, CommandListId cmd_list_id) const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).HasListWithState(cmd_list_state, cmd_list_id);
}

RenderCommandList CommandKit::GetRenderList(CommandListId cmd_list_id) const
{
    META_CHECK_ARG_EQUAL(GetListType(), CommandListType::Render);
    return RenderCommandList(dynamic_cast<IRenderCommandList&>(GetPrivateImpl(m_impl_ptr).GetList(cmd_list_id)));
}

RenderCommandList CommandKit::GetRenderListForEncoding(CommandListId cmd_list_id, std::string_view debug_group_name) const
{
    META_CHECK_ARG_EQUAL(GetListType(), CommandListType::Render);
    return RenderCommandList(dynamic_cast<IRenderCommandList&>(GetPrivateImpl(m_impl_ptr).GetListForEncoding(cmd_list_id, debug_group_name)));
}

CommandListSet CommandKit::GetListSet(const std::vector<CommandListId>& cmd_list_ids, Opt<Data::Index> frame_index_opt) const
{
    return CommandListSet(GetPrivateImpl(m_impl_ptr).GetListSet(cmd_list_ids, frame_index_opt));
}

IFence& CommandKit::GetFence(CommandListId fence_id) const
{
    return GetPrivateImpl(m_impl_ptr).GetFence(fence_id);
}

} // namespace Methane::Graphics::Rhi
