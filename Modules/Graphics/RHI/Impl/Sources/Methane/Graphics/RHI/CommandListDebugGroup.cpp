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

#if defined METHANE_GFX_DIRECTX

#include <Methane/Graphics/DirectX/ICommandList.h>
using CommandListDebugGroupImpl = Methane::Graphics::DirectX::CommandListDebugGroup;

#elif defined METHANE_GFX_VULKAN

#include <Methane/Graphics/Vulkan/ICommandList.h>
using CommandListDebugGroupImpl = Methane::Graphics::Vulkan::CommandListDebugGroup;

#elif defined METHANE_GFX_METAL

#include <Methane/Graphics/Metal/CommandList.hh>
using CommandListDebugGroupImpl = Methane::Graphics::Metal::CommandListDebugGroup;

#else // METHAN_GFX_[API] is undefined

static_assert(false, "Static graphics API macro-definition is missing.");

#endif

#include "ImplWrapper.hpp"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

class CommandListDebugGroup::Impl
    : public ImplWrapper<ICommandListDebugGroup, CommandListDebugGroupImpl>
{
public:
    using ImplWrapper::ImplWrapper;
};

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(CommandListDebugGroup);

CommandListDebugGroup::CommandListDebugGroup(const Ptr<ICommandListDebugGroup>& interface_ptr)
    : m_impl_ptr(std::make_unique<Impl>(interface_ptr))
{
}

CommandListDebugGroup::CommandListDebugGroup(ICommandListDebugGroup& interface_ref)
    : CommandListDebugGroup(std::dynamic_pointer_cast<ICommandListDebugGroup>(interface_ref.GetPtr()))
{
}

CommandListDebugGroup::CommandListDebugGroup(std::string_view name)
    : CommandListDebugGroup(ICommandListDebugGroup::Create(name))
{
}

void CommandListDebugGroup::Init(std::string_view name)
{
    m_impl_ptr = std::make_unique<Impl>(ICommandListDebugGroup::Create(name));
}

void CommandListDebugGroup::Release()
{
    m_impl_ptr.release();
}

bool CommandListDebugGroup::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

ICommandListDebugGroup& CommandListDebugGroup::GetInterface() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterface(m_impl_ptr);
}

bool CommandListDebugGroup::SetName(const std::string& name) const
{
    return GetPrivateImpl(m_impl_ptr).SetName(name);
}

const std::string& CommandListDebugGroup::GetName() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetName();
}

CommandListDebugGroup CommandListDebugGroup::AddSubGroup(Data::Index id, const std::string& name)
{
    return CommandListDebugGroup(GetPrivateImpl(m_impl_ptr).AddSubGroup(id, name));
}

Opt<CommandListDebugGroup> CommandListDebugGroup::GetSubGroup(Data::Index id) const META_PIMPL_NOEXCEPT
{
    if (ICommandListDebugGroup* sub_group_ptr = GetPrivateImpl(m_impl_ptr).GetSubGroup(id);
        sub_group_ptr)
        return CommandListDebugGroup(*sub_group_ptr);

    return std::nullopt;
}

bool CommandListDebugGroup::HasSubGroups() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).HasSubGroups();
}

} // namespace Methane::Graphics::Rhi
