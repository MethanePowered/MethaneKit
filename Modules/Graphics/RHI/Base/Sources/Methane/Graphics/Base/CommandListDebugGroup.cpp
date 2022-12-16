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

FILE: Methane/Graphics/Base/CommandListDebugGroup.cpp
Base implementation of the command list debug group interface.

******************************************************************************/

#include <Methane/Graphics/Base/CommandListDebugGroup.h>

namespace Methane::Graphics::Base
{

CommandListDebugGroup::CommandListDebugGroup(std::string_view name)
    : Object(name)
{
    META_FUNCTION_TASK();
}

bool CommandListDebugGroup::SetName(std::string_view)
{
    META_FUNCTION_NOT_IMPLEMENTED_RETURN_DESCR(false, "Debug Group can not be renamed");
}

Rhi::ICommandListDebugGroup& CommandListDebugGroup::AddSubGroup(Data::Index id, const std::string& name)
{
    META_FUNCTION_TASK();
    if (id >= m_sub_groups.size())
    {
        m_sub_groups.resize(id + 1);
    }

    Ptr<ICommandListDebugGroup> sub_group_ptr = Rhi::ICommandListDebugGroup::Create(name);
    m_sub_groups[id] = sub_group_ptr;
    return *sub_group_ptr;
}

Rhi::ICommandListDebugGroup* CommandListDebugGroup::GetSubGroup(Data::Index id) const noexcept
{
    META_FUNCTION_TASK();
    return id < m_sub_groups.size() ? m_sub_groups[id].get() : nullptr;
}

} // namespace Methane::Graphics::Base
