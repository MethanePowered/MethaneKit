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

FILE: Methane/Graphics/RHI/ProgramBindings.cpp
Methane ProgramBindings PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#include <Methane/Graphics/RHI/ProgramBindings.h>
#include <Methane/Graphics/RHI/Program.h>

#if defined METHANE_GFX_DIRECTX

#include <Methane/Graphics/DirectX/ProgramBindings.h>
using ProgramBindingsImpl = Methane::Graphics::DirectX::ProgramBindings;

#elif defined METHANE_GFX_VULKAN

#include <Methane/Graphics/Vulkan/ProgramBindings.h>
using ProgramBindingsImpl = Methane::Graphics::Vulkan::ProgramBindings;

#elif defined METHANE_GFX_METAL

#include <Methane/Graphics/Metal/ProgramBindings.hh>
using ProgramBindingsImpl = Methane::Graphics::Metal::ProgramBindings;

#else // METHAN_GFX_[API] is undefined

static_assert(false, "Static graphics API macro-definition is missing.");

#endif

#include "ImplWrapper.hpp"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

class ProgramBindings::Impl
    : public ImplWrapper<IProgramBindings, ProgramBindingsImpl>
{
public:
    using ImplWrapper::ImplWrapper;
};

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(ProgramBindings);

ProgramBindings::ProgramBindings(ImplPtr<Impl>&& impl_ptr)
    : Transmitter(impl_ptr->GetInterface())
    , m_impl_ptr(std::move(impl_ptr))
{
}

ProgramBindings::ProgramBindings(const Ptr<IProgramBindings>& interface_ptr)
    : ProgramBindings(std::make_unique<Impl>(interface_ptr))
{
}

ProgramBindings::ProgramBindings(IProgramBindings& interface_ref)
    : ProgramBindings(interface_ref.GetDerivedPtr<IProgramBindings>())
{
}

ProgramBindings::ProgramBindings(const Program& program, const ResourceViewsByArgument& resource_views_by_argument, Data::Index frame_index)
    : ProgramBindings(IProgramBindings::Create(program.GetInterface(), resource_views_by_argument, frame_index))
{
}

ProgramBindings::ProgramBindings(const ProgramBindings& other_program_bindings, const ResourceViewsByArgument& replace_resource_views_by_argument,
                                 const Opt<Data::Index>& frame_index)
    : ProgramBindings(IProgramBindings::CreateCopy(other_program_bindings.GetInterface(), replace_resource_views_by_argument, frame_index))
{
}

void ProgramBindings::Init(const Program& program, const ResourceViewsByArgument& resource_views_by_argument, Data::Index frame_index)
{
    m_impl_ptr = std::make_unique<Impl>(IProgramBindings::Create(program.GetInterface(), resource_views_by_argument, frame_index));
    Transmitter::Reset(&m_impl_ptr->GetInterface());
}

void ProgramBindings::InitCopy(const ProgramBindings& other_program_bindings, const ResourceViewsByArgument& replace_resource_views_by_argument,
                               const Opt<Data::Index>& frame_index)
{
    m_impl_ptr = std::make_unique<Impl>(IProgramBindings::CreateCopy(other_program_bindings.GetInterface(), replace_resource_views_by_argument, frame_index));
    Transmitter::Reset(&m_impl_ptr->GetInterface());
}

void ProgramBindings::Release()
{
    Transmitter::Reset();
    m_impl_ptr.reset();
}

bool ProgramBindings::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

IProgramBindings& ProgramBindings::GetInterface() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterface(m_impl_ptr);
}

Ptr<IProgramBindings> ProgramBindings::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterfacePtr(m_impl_ptr);
}

bool ProgramBindings::SetName(std::string_view name) const
{
    return GetPrivateImpl(m_impl_ptr).SetName(name);
}

std::string_view ProgramBindings::GetName() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetName();
}

Program ProgramBindings::GetProgram() const
{
    return Program(GetPrivateImpl(m_impl_ptr).GetProgram());
}

IProgramArgumentBinding& ProgramBindings::Get(const ProgramArgument& shader_argument) const
{
    return GetPrivateImpl(m_impl_ptr).Get(shader_argument);
}

const ProgramArguments& ProgramBindings::GetArguments() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetArguments();
}

Data::Index ProgramBindings::GetFrameIndex() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetFrameIndex();
}

Data::Index ProgramBindings::GetBindingsIndex() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetBindingsIndex();
}

ProgramBindings::operator std::string() const
{
    return GetPrivateImpl(m_impl_ptr).operator std::string();
}

} // namespace Methane::Graphics::Rhi
