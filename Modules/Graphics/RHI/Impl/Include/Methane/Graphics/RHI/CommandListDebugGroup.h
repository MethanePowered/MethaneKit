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

#include <Methane/Pimpl.h>

#include <Methane/Graphics/RHI/ICommandList.h>

namespace Methane::Graphics::META_GFX_NAME
{
class CommandListDebugGroup;
}

namespace Methane::Graphics::Rhi
{

class CommandQueue;
class RenderPass;

class CommandListDebugGroup // NOSONAR - constructors and assignment operators are required to use forward declared Impl and Ptr<Impl> in header
{
public:
    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(CommandListDebugGroup);
    META_PIMPL_METHODS_COMPARE_DECLARE(CommandListDebugGroup);

    META_PIMPL_API explicit CommandListDebugGroup(const Ptr<ICommandListDebugGroup>& interface_ptr);
    META_PIMPL_API explicit CommandListDebugGroup(ICommandListDebugGroup& interface_ref);
    META_PIMPL_API explicit CommandListDebugGroup(std::string_view name);

    META_PIMPL_API bool IsInitialized() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API ICommandListDebugGroup& GetInterface() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API Ptr<ICommandListDebugGroup> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    META_PIMPL_API bool SetName(std::string_view name) const;
    META_PIMPL_API std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // Data::IEmitter<IObjectCallback> interface methods
    META_PIMPL_API void Connect(Data::Receiver<IObjectCallback>& receiver) const;
    META_PIMPL_API void Disconnect(Data::Receiver<IObjectCallback>& receiver) const;

    // ICommandListDebugGroup interface methods
    META_PIMPL_API CommandListDebugGroup AddSubGroup(Data::Index id, const std::string& name) const;
    [[nodiscard]] META_PIMPL_API Opt<CommandListDebugGroup> GetSubGroup(Data::Index id) const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API bool HasSubGroups() const META_PIMPL_NOEXCEPT;

private:
    using Impl = Methane::Graphics::META_GFX_NAME::CommandListDebugGroup;

    Ptr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi

#ifdef META_PIMPL_INLINE

#include <Methane/Graphics/RHI/CommandListDebugGroup.cpp>

#endif // META_PIMPL_INLINE

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