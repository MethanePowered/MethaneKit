/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/ComputeState.cpp
Methane ComputeState PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#include <Methane/Graphics/RHI/ComputeState.h>
#include <Methane/Graphics/RHI/RenderContext.h>
#include <Methane/Graphics/RHI/ComputeContext.h>

#include <Methane/Pimpl.hpp>

#ifdef META_GFX_METAL
#include <ComputeState.hh>
#else
#include <ComputeState.h>
#endif

namespace Methane::Graphics::Rhi
{

ComputeStateSettings ComputeStateSettingsImpl::Convert(const ComputeStateSettingsImpl& settings)
{
    return ComputeStateSettings
    {
        settings.program.GetInterfacePtr(),
        settings.thread_group_size
    };
}

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(ComputeState);

ComputeState::ComputeState(const Ptr<IComputeState>& interface_ptr)
    : m_impl_ptr(std::dynamic_pointer_cast<Impl>(interface_ptr))
{
}

ComputeState::ComputeState(IComputeState& interface_ref)
    : ComputeState(interface_ref.GetDerivedPtr<IComputeState>())
{
}

ComputeState::ComputeState(const RenderContext& context, const Settings& settings)
    : ComputeState(IComputeState::Create(context.GetInterface(), ComputeStateSettingsImpl::Convert(settings)))
{
}

ComputeState::ComputeState(const ComputeContext& context, const Settings& settings)
    : ComputeState(IComputeState::Create(context.GetInterface(), ComputeStateSettingsImpl::Convert(settings)))
{
}

bool ComputeState::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

IComputeState& ComputeState::GetInterface() const META_PIMPL_NOEXCEPT
{
    return *m_impl_ptr;
}

Ptr<IComputeState> ComputeState::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return m_impl_ptr;
}

bool ComputeState::SetName(std::string_view name) const
{
    return GetImpl(m_impl_ptr).SetName(name);
}

std::string_view ComputeState::GetName() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetName();
}

void ComputeState::Connect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Connect(receiver);
}

void ComputeState::Disconnect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Disconnect(receiver);
}

const ComputeStateSettings& ComputeState::GetSettings() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetSettings();
}

void ComputeState::Reset(const Settings& settings) const
{
    return GetImpl(m_impl_ptr).Reset(ComputeStateSettingsImpl::Convert(settings));
}

void ComputeState::Reset(const IComputeState::Settings& settings) const
{
    return GetImpl(m_impl_ptr).Reset(settings);
}

Program ComputeState::GetProgram() const
{
    return Program(GetSettings().program_ptr);
}

} // namespace Methane::Graphics::Rhi
