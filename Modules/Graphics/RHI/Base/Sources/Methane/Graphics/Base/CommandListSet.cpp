/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Base/CommandListSet.cpp
Base implementation of the command list set interface.

******************************************************************************/

#include <Methane/Graphics/Base/CommandListSet.h>
#include <Methane/Graphics/Base/Device.h>
#include <Methane/Graphics/Base/CommandQueue.h>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <sstream>

namespace Methane::Graphics::Base
{

CommandListSet::CommandListSet(const Refs<Rhi::ICommandList>& command_list_refs, Opt<Data::Index> frame_index_opt)
    : m_refs(command_list_refs)
    , m_frame_index_opt(frame_index_opt)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_EMPTY_DESCR(command_list_refs, "creating of empty command lists set is not allowed.");

    m_base_refs.reserve(m_refs.size());
    m_base_ptrs.reserve(m_refs.size());

    for(const Ref<Rhi::ICommandList>& command_list_ref : m_refs)
    {
        auto& command_list_base = dynamic_cast<CommandList&>(command_list_ref.get());
        META_CHECK_ARG_NAME_DESCR("command_list_refs",
                                  std::addressof(command_list_base.GetCommandQueue()) == std::addressof(m_refs.front().get().GetCommandQueue()),
                                  "all command lists in set must be created in one command queue");

        static_cast<Data::IEmitter<IObjectCallback>&>(command_list_base).Connect(*this);

        m_base_refs.emplace_back(command_list_base);
        m_base_ptrs.emplace_back(command_list_base.GetCommandListPtr());
    }
}

Rhi::ICommandList& CommandListSet::operator[](Data::Index index) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS(index, m_refs.size());

    return m_refs[index].get();
}

void CommandListSet::Execute(const Rhi::ICommandList::CompletedCallback& completed_callback)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_command_lists_mutex);

    m_is_executing = true;

    for (const Ref<CommandList>& command_list_ref : m_base_refs)
    {
        command_list_ref.get().Execute(completed_callback);
    }
}

void CommandListSet::Complete() const
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_command_lists_mutex);

    for (const Ref<CommandList>& command_list_ref : m_base_refs)
    {
        CommandList& command_list = command_list_ref.get();
        if (command_list.GetState() != CommandList::State::Executing)
            continue;

        command_list.Complete();
    }

    m_is_executing = false;
}

const CommandList& CommandListSet::GetBaseCommandList(Data::Index index) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS(index, m_base_refs.size());
    return m_base_refs[index].get();
}

const std::string& CommandListSet::GetCombinedName()
{
    META_FUNCTION_TASK();
    if (!m_combined_name.empty())
        return m_combined_name;

    std::stringstream name_ss;
    const size_t list_count = m_refs.size();
    name_ss << list_count << " Command List" << (list_count > 1 ? "s: " : ": ");

    for (size_t list_index = 0u; list_index < list_count; ++list_index)
    {
        if (const std::string_view list_name = m_refs[list_index].get().GetName();
            list_name.empty())
            name_ss << "<unnamed>";
        else
            name_ss << "'" << list_name << "'";

        if (list_index < list_count - 1)
            name_ss << ", ";
    }

    m_combined_name = name_ss.str();
    return m_combined_name;
}

void CommandListSet::OnObjectNameChanged(Rhi::IObject&, const std::string&)
{
    META_FUNCTION_TASK();
    m_combined_name.clear();
}

} // namespace Methane::Graphics::Base
