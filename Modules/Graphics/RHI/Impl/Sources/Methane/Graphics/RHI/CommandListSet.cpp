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

FILE: Methane/Graphics/RHI/CommandListSet.cpp
Methane CommandListSet PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#include <Methane/Graphics/RHI/CommandListSet.h>
#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/RenderPass.h>

#include "Pimpl.hpp"

#ifdef META_GFX_METAL
#include <CommandListSet.hh>
#else
#include <CommandListSet.h>
#endif

namespace Methane::Graphics::Rhi
{

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(CommandListSet);
META_PIMPL_METHODS_COMPARE_IMPLEMENT(CommandListSet);

CommandListSet::CommandListSet(const Ptr<ICommandListSet>& interface_ptr)
    : m_impl_ptr(std::dynamic_pointer_cast<Impl>(interface_ptr))
{
}

CommandListSet::CommandListSet(ICommandListSet& interface_ref)
    : CommandListSet(interface_ref.GetPtr())
{
}

CommandListSet::CommandListSet(const Refs<ICommandList>& command_list_refs, Opt<Data::Index> frame_index_opt)
    : CommandListSet(ICommandListSet::Create(command_list_refs, frame_index_opt))
{
}

bool CommandListSet::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

ICommandListSet& CommandListSet::GetInterface() const META_PIMPL_NOEXCEPT
{
    return *m_impl_ptr;
}

Ptr<ICommandListSet> CommandListSet::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return m_impl_ptr;
}

Data::Size CommandListSet::GetCount() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetCount();
}

const Refs<ICommandList>& CommandListSet::GetRefs() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetRefs();
}

ICommandList& CommandListSet::operator[](Data::Index index) const
{
    return GetImpl(m_impl_ptr)[index];
}

const Opt<Data::Index>& CommandListSet::GetFrameIndex() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetFrameIndex();
}

} // namespace Methane::Graphics::Rhi
