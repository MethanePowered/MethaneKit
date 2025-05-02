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

FILE: Methane/Graphics/RHI/RenderState.cpp
Methane RenderState PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#include <Methane/Graphics/RHI/RenderState.h>
#include <Methane/Graphics/RHI/RenderContext.h>

#include <Methane/Pimpl.hpp>

#ifdef META_GFX_METAL
#include <RenderState.hh>
#else
#include <RenderState.h>
#endif

namespace Methane::Graphics::Rhi
{

RenderStateSettings RenderStateSettingsImpl::Convert(const RenderStateSettingsImpl& settings)
{
    return RenderStateSettings
    {
        settings.program.GetInterfacePtr(),
        settings.render_pattern.GetInterfacePtr(),
        settings.rasterizer,
        settings.depth,
        settings.stencil,
        settings.blending,
        settings.blending_color
    };
}

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(RenderState);

RenderState::RenderState(const Ptr<IRenderState>& interface_ptr)
    : m_impl_ptr(std::dynamic_pointer_cast<Impl>(interface_ptr))
{
}

RenderState::RenderState(IRenderState& interface_ref)
    : RenderState(interface_ref.GetDerivedPtr<IRenderState>())
{
}

RenderState::RenderState(const RenderContext& context, const Settings& settings)
    : RenderState(IRenderState::Create(context.GetInterface(), RenderStateSettingsImpl::Convert(settings)))
{
}

bool RenderState::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

IRenderState& RenderState::GetInterface() const META_PIMPL_NOEXCEPT
{
    return *m_impl_ptr;
}

Ptr<IRenderState> RenderState::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return m_impl_ptr;
}

bool RenderState::SetName(std::string_view name) const
{
    return GetImpl(m_impl_ptr).SetName(name);
}

std::string_view RenderState::GetName() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetName();
}

void RenderState::Connect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Connect(receiver);
}

void RenderState::Disconnect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Disconnect(receiver);
}

const RenderStateSettings& RenderState::GetSettings() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetSettings();
}

void RenderState::Reset(const Settings& settings) const
{
    return GetImpl(m_impl_ptr).Reset(RenderStateSettingsImpl::Convert(settings));
}

void RenderState::Reset(const IRenderState::Settings& settings) const
{
    return GetImpl(m_impl_ptr).Reset(settings);
}

Program RenderState::GetProgram() const
{
    return Program(GetSettings().program_ptr);
}

RenderPattern RenderState::GetRenderPattern() const
{
    return RenderPattern(GetSettings().render_pattern_ptr);
}

} // namespace Methane::Graphics::Rhi
