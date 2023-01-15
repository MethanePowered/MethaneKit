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

FILE: Methane/Graphics/RHI/RenderPass.cpp
Methane RenderPass PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#include <Methane/Graphics/RHI/RenderPass.h>
#include <Methane/Graphics/RHI/RenderPattern.h>
#include <Methane/Graphics/RHI/RenderContext.h>

#include "Pimpl.hpp"

#ifdef META_GFX_METAL
#include <RenderPass.hh>
#else
#include <RenderPass.h>
#endif

namespace Methane::Graphics::Rhi
{

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(RenderPass);
META_PIMPL_METHODS_COMPARE_IMPLEMENT(RenderPass);

RenderPass::RenderPass(const Ptr<IRenderPass>& interface_ptr)
    : m_impl_ptr(std::dynamic_pointer_cast<Impl>(interface_ptr))
{
}

RenderPass::RenderPass(IRenderPass& interface_ref)
    : RenderPass(interface_ref.GetDerivedPtr<IRenderPass>())
{
}

RenderPass::RenderPass(const Pattern& render_pattern, const Settings& settings)
    : RenderPass(IRenderPass::Create(render_pattern.GetInterface(), settings))
{
}

bool RenderPass::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

IRenderPass& RenderPass::GetInterface() const META_PIMPL_NOEXCEPT
{
    return *m_impl_ptr;
}

Ptr<IRenderPass> RenderPass::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return m_impl_ptr;
}

bool RenderPass::SetName(std::string_view name) const
{
    return GetImpl(m_impl_ptr).SetName(name);
}

std::string_view RenderPass::GetName() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetName();
}

void RenderPass::Connect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Connect(receiver);
}

void RenderPass::Disconnect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Disconnect(receiver);
}

RenderPattern RenderPass::GetPattern() const META_PIMPL_NOEXCEPT
{
    return RenderPattern(GetImpl(m_impl_ptr).GetPattern());
}

const RenderPassSettings& RenderPass::GetSettings() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetSettings();
}

bool RenderPass::Update(const Settings& settings) const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).Update(settings);
}

void RenderPass::ReleaseAttachmentTextures() const META_PIMPL_NOEXCEPT
{
    GetImpl(m_impl_ptr).ReleaseAttachmentTextures();
}

void RenderPass::Connect(Data::Receiver<IRenderPassCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IRenderPassCallback>::Connect(receiver);
}

void RenderPass::Disconnect(Data::Receiver<IRenderPassCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IRenderPassCallback>::Disconnect(receiver);
}

} // namespace Methane::Graphics::Rhi
