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

FILE: Methane/Graphics/RHI/RenderPattern.cpp
Methane RenderPattern PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#include <Methane/Graphics/RHI/RenderPattern.h>
#include <Methane/Graphics/RHI/RenderPass.h>
#include <Methane/Graphics/RHI/RenderContext.h>

#include <Methane/Pimpl.hpp>

#ifdef META_GFX_METAL
#include <RenderPattern.hh>
#else
#include <RenderPattern.h>
#endif

namespace Methane::Graphics::Rhi
{

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(RenderPattern);
META_PIMPL_METHODS_COMPARE_IMPLEMENT(RenderPattern);

RenderPattern::RenderPattern(const Ptr<IRenderPattern>& interface_ptr)
    : m_impl_ptr(std::dynamic_pointer_cast<Impl>(interface_ptr))
{
}

RenderPattern::RenderPattern(IRenderPattern& interface_ref)
    : RenderPattern(interface_ref.GetDerivedPtr<IRenderPattern>())
{
}

RenderPattern::RenderPattern(const RenderContext& render_context, const Settings& settings)
    : RenderPattern(IRenderPattern::Create(render_context.GetInterface(), settings))
{
}

bool RenderPattern::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

IRenderPattern& RenderPattern::GetInterface() const META_PIMPL_NOEXCEPT
{
    return *m_impl_ptr;
}

Ptr<IRenderPattern> RenderPattern::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return m_impl_ptr;
}

bool RenderPattern::SetName(std::string_view name) const
{
    return GetImpl(m_impl_ptr).SetName(name);
}

std::string_view RenderPattern::GetName() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetName();
}

void RenderPattern::Connect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Connect(receiver);
}

void RenderPattern::Disconnect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Disconnect(receiver);
}

RenderPass RenderPattern::CreateRenderPass(const RenderPassSettings& settings) const
{
    return RenderPass(GetImpl(m_impl_ptr).CreateRenderPass(settings));
}

RenderContext RenderPattern::GetRenderContext() const META_PIMPL_NOEXCEPT
{
    return RenderContext(GetImpl(m_impl_ptr).GetRenderContext());
}

const RenderPatternSettings& RenderPattern::GetSettings() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetSettings();
}

Data::Size RenderPattern::GetAttachmentCount() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetAttachmentCount();
}

AttachmentFormats RenderPattern::GetAttachmentFormats() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetAttachmentFormats();
}

} // namespace Methane::Graphics::Rhi
