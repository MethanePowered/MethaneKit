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


#if defined METHANE_GFX_DIRECTX

#include <Methane/Graphics/DirectX/CommandListSet.h>
using CommandListSetImpl = Methane::Graphics::DirectX::CommandListSet;

#elif defined METHANE_GFX_VULKAN

#include <Methane/Graphics/Vulkan/CommandListSet.h>
using CommandListSetImpl = Methane::Graphics::Vulkan::CommandListSet;

#elif defined METHANE_GFX_METAL

#include <Methane/Graphics/Metal/CommandList.hh>
using CommandListSetImpl = Methane::Graphics::Metal::CommandListSet;

#else // METHAN_GFX_[API] is undefined

static_assert(false, "Static graphics API macro-definition is missing.");

#endif

#include "ImplWrapper.hpp"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

class CommandListSet::Impl
    : public ImplWrapper<ICommandListSet, CommandListSetImpl>
{
public:
    using ImplWrapper::ImplWrapper;
};

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(CommandListSet);

CommandListSet::CommandListSet(const Ptr<ICommandListSet>& interface_ptr)
    : m_impl_ptr(std::make_unique<Impl>(interface_ptr))
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

void CommandListSet::Init(const Refs<ICommandList>& command_list_refs, Opt<Data::Index> frame_index_opt)
{
    m_impl_ptr = std::make_unique<Impl>(ICommandListSet::Create(command_list_refs, frame_index_opt));
}

void CommandListSet::Release()
{
    m_impl_ptr.release();
}

bool CommandListSet::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

ICommandListSet& CommandListSet::GetInterface() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterface(m_impl_ptr);
}

Data::Size CommandListSet::GetCount() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetCount();
}

const Refs<ICommandList>& CommandListSet::GetRefs() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetRefs();
}

ICommandList& CommandListSet::operator[](Data::Index index) const
{
    return GetPrivateImpl(m_impl_ptr)[index];
}

const Opt<Data::Index>& CommandListSet::GetFrameIndex() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetFrameIndex();
}

} // namespace Methane::Graphics::Rhi
