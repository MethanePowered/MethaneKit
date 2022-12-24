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

FILE: Methane/Graphics/RHI/CommandListDebugGroup.h
Methane CommandListDebugGroup PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include "Pimpl.h"

#include <Methane/Graphics/RHI/ICommandList.h>
#include <Methane/Data/Transmitter.hpp>

namespace Methane::Graphics::Rhi
{

class CommandQueue;
class RenderPass;

class CommandListDebugGroup
    : Data::Transmitter<IObjectCallback>
{
public:
    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(CommandListDebugGroup);

    CommandListDebugGroup(const Ptr<ICommandListDebugGroup>& interface_ptr);
    CommandListDebugGroup(ICommandListDebugGroup& interface_ref);
    CommandListDebugGroup(std::string_view name);

    void Init(std::string_view name);
    void Release();

    bool IsInitialized() const META_PIMPL_NOEXCEPT;
    ICommandListDebugGroup& GetInterface() const META_PIMPL_NOEXCEPT;
    Ptr<ICommandListDebugGroup> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    bool SetName(std::string_view name) const;
    std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // ICommandListDebugGroup interface methods
    CommandListDebugGroup AddSubGroup(Data::Index id, const std::string& name);
    [[nodiscard]] Opt<CommandListDebugGroup> GetSubGroup(Data::Index id) const META_PIMPL_NOEXCEPT;
    [[nodiscard]] bool HasSubGroups() const META_PIMPL_NOEXCEPT;

private:
    class Impl;

    CommandListDebugGroup(ImplPtr<Impl>&& impl_ptr);

    ImplPtr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi

#ifdef METHANE_COMMAND_DEBUG_GROUPS_ENABLED

#define META_DEBUG_GROUP_VAR(variable, /*const std::string& */group_name) \
    const Methane::Graphics::Rhi::CommandListDebugGroup variable(group_name)

#define META_DEBUG_GROUP_VAR_PUSH(/*ICommandList& */cmd_list, /*const std::string& */group_name) \
    { \
        META_DEBUG_GROUP_VAR(s_local_debug_group, group_name); \
        (cmd_list).PushDebugGroup(s_local_debug_group); \
    }

#else

#define META_DEBUG_GROUP_VAR(variable, /*const std::string& */group_name) \
    nullptr

#define META_DEBUG_GROUP_VAR_PUSH(/*ICommandList& */cmd_list, /*const std::string& */group_name) \
    META_UNUSED(cmd_list); META_UNUSED(group_name)

#endif