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

FILE: Methane/Graphics/RHI/ICommandListDebugGroup.h
Methane command list debug group interface.

******************************************************************************/

#pragma once

#include "IObject.h"

#include <Methane/Data/Types.h>

#include <string_view>

namespace Methane::Graphics::Rhi
{

struct ICommandListDebugGroup
    : virtual IObject // NOSONAR
{
    [[nodiscard]] static Ptr<ICommandListDebugGroup> Create(std::string_view name);

    virtual ICommandListDebugGroup& AddSubGroup(Data::Index id, std::string_view name) = 0;
    [[nodiscard]] virtual ICommandListDebugGroup* GetSubGroup(Data::Index id) const noexcept = 0;
    [[nodiscard]] virtual bool HasSubGroups() const noexcept = 0;
};

} // namespace Methane::Graphics::Rhi

#ifdef METHANE_COMMAND_DEBUG_GROUPS_ENABLED

#define META_DEBUG_GROUP_CREATE(/*std::string_view*/group_name) \
    Methane::Graphics::Rhi::ICommandListDebugGroup::Create(group_name)

#define META_DEBUG_GROUP_PUSH(/*ICommandList& */cmd_list, /*std::string_view*/group_name) \
    { \
        const auto s_local_debug_group = META_DEBUG_GROUP_CREATE(group_name); \
        (cmd_list).PushDebugGroup(*s_local_debug_group); \
    }

#define META_DEBUG_GROUP_POP(/*ICommandList& */cmd_list) \
    (cmd_list).PopDebugGroup()

#else

#define META_DEBUG_GROUP_CREATE(/*std::string_view*/group_name) \
    Methane::Ptr<Methane::Graphics::Rhi::ICommandListDebugGroup>()

#define META_DEBUG_GROUP_PUSH(/*ICommandList& */cmd_list, /*std::string_view*/group_name) \
    META_UNUSED(cmd_list); META_UNUSED(group_name)

#define META_DEBUG_GROUP_POP(/*ICommandList& */cmd_list) \
    META_UNUSED(cmd_list)

#endif

#define META_DEBUG_GROUP_CREATE_VAR(variable, /*std::string_view*/ group_name) \
    META_UNUSED(group_name); \
    static const Methane::Ptr<Methane::Graphics::Rhi::ICommandListDebugGroup> variable = META_DEBUG_GROUP_CREATE(group_name)
