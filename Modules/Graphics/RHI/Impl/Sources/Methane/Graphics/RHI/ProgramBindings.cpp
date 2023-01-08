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

#include "Pimpl.hpp"

#ifdef META_GFX_METAL
#include <ProgramBindings.hh>
#else
#include <ProgramBindings.h>
#endif

namespace Methane::Graphics::Rhi
{

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(ProgramBindings);
META_PIMPL_METHODS_COMPARE_IMPLEMENT(ProgramBindings);

ProgramBindings::ProgramBindings(Ptr<Impl>&& impl_ptr)
    : m_impl_ptr(std::move(impl_ptr))
{
}

ProgramBindings::ProgramBindings(const Ptr<IProgramBindings>& interface_ptr)
    : ProgramBindings(std::dynamic_pointer_cast<Impl>(interface_ptr))
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
    : ProgramBindings(other_program_bindings.GetInterface().CreateCopy(replace_resource_views_by_argument, frame_index))
{
}

bool ProgramBindings::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

IProgramBindings& ProgramBindings::GetInterface() const META_PIMPL_NOEXCEPT
{
    return *m_impl_ptr;
}

Ptr<IProgramBindings> ProgramBindings::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return m_impl_ptr;
}

bool ProgramBindings::SetName(std::string_view name) const
{
    return GetImpl(m_impl_ptr).SetName(name);
}

std::string_view ProgramBindings::GetName() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetName();
}

void ProgramBindings::Connect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Connect(receiver);
}

void ProgramBindings::Disconnect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Disconnect(receiver);
}

Program ProgramBindings::GetProgram() const
{
    return Program(GetImpl(m_impl_ptr).GetProgram());
}

IProgramArgumentBinding& ProgramBindings::Get(const ProgramArgument& shader_argument) const
{
    return GetImpl(m_impl_ptr).Get(shader_argument);
}

const ProgramArguments& ProgramBindings::GetArguments() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetArguments();
}

Data::Index ProgramBindings::GetFrameIndex() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetFrameIndex();
}

Data::Index ProgramBindings::GetBindingsIndex() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetBindingsIndex();
}

ProgramBindings::operator std::string() const
{
    return GetImpl(m_impl_ptr).operator std::string();
}

} // namespace Methane::Graphics::Rhi
