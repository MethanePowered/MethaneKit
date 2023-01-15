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

FILE: Methane/Graphics/RHI/CommandListDebugGroup.cpp
Methane CommandListDebugGroup PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#include <Methane/Graphics/RHI/CommandListDebugGroup.h>
#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/RenderPass.h>

#include "Pimpl.hpp"

#ifdef META_GFX_METAL
#include <CommandListDebugGroup.hh>
#else
#include <CommandListDebugGroup.h>
#endif

namespace Methane::Graphics::Rhi
{

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(CommandListDebugGroup);
META_PIMPL_METHODS_COMPARE_IMPLEMENT(CommandListDebugGroup);

CommandListDebugGroup::CommandListDebugGroup(const Ptr<ICommandListDebugGroup>& interface_ptr)
    : m_impl_ptr(std::dynamic_pointer_cast<Impl>(interface_ptr))
{
}

CommandListDebugGroup::CommandListDebugGroup(ICommandListDebugGroup& interface_ref)
    : CommandListDebugGroup(interface_ref.GetDerivedPtr<ICommandListDebugGroup>())
{
}

CommandListDebugGroup::CommandListDebugGroup(std::string_view name)
    : CommandListDebugGroup(ICommandListDebugGroup::Create(name))
{
}

bool CommandListDebugGroup::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

ICommandListDebugGroup& CommandListDebugGroup::GetInterface() const META_PIMPL_NOEXCEPT
{
    return *m_impl_ptr;
}

Ptr<ICommandListDebugGroup> CommandListDebugGroup::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return m_impl_ptr;
}

bool CommandListDebugGroup::SetName(std::string_view name) const
{
    return GetImpl(m_impl_ptr).SetName(name);
}

std::string_view CommandListDebugGroup::GetName() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetName();
}

void CommandListDebugGroup::Connect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Connect(receiver);
}

void CommandListDebugGroup::Disconnect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Disconnect(receiver);
}

CommandListDebugGroup CommandListDebugGroup::AddSubGroup(Data::Index id, const std::string& name) const
{
    return CommandListDebugGroup(GetImpl(m_impl_ptr).AddSubGroup(id, name));
}

Opt<CommandListDebugGroup> CommandListDebugGroup::GetSubGroup(Data::Index id) const META_PIMPL_NOEXCEPT
{
    if (ICommandListDebugGroup* sub_group_ptr = GetImpl(m_impl_ptr).GetSubGroup(id);
        sub_group_ptr)
        return CommandListDebugGroup(*sub_group_ptr);

    return std::nullopt;
}

bool CommandListDebugGroup::HasSubGroups() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).HasSubGroups();
}

} // namespace Methane::Graphics::Rhi
