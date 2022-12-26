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
#include <Methane/Graphics/RHI/RenderContext.h>

#if defined METHANE_GFX_METAL
#include <RenderPass.hh>
#else
#include <RenderPass.h>
#endif

#include "Pimpl.hpp"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(RenderPattern);
META_PIMPL_METHODS_COMPARE_IMPLEMENT(RenderPattern);

RenderPattern::RenderPattern(Ptr<Impl>&& impl_ptr)
    : Transmitter(*impl_ptr)
    , m_impl_ptr(std::move(impl_ptr))
{
}

RenderPattern::RenderPattern(const Ptr<IRenderPattern>& interface_ptr)
    : RenderPattern(std::dynamic_pointer_cast<Impl>(interface_ptr))
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

void RenderPattern::Init(const RenderContext& render_context, const Settings& settings)
{
    m_impl_ptr = std::dynamic_pointer_cast<Impl>(IRenderPattern::Create(render_context.GetInterface(), settings));
    Transmitter::Reset(m_impl_ptr.get());
}

void RenderPattern::Release()
{
    Transmitter::Reset();
    m_impl_ptr.reset();
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

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(RenderPass);
META_PIMPL_METHODS_COMPARE_IMPLEMENT(RenderPass);

RenderPass::RenderPass(Ptr<Impl>&& impl_ptr)
    : Transmitter<IObjectCallback>(*impl_ptr)
    , Transmitter<IRenderPassCallback>(*impl_ptr)
    , m_impl_ptr(std::move(impl_ptr))
{
}

RenderPass::RenderPass(const Ptr<IRenderPass>& interface_ptr)
    : RenderPass(std::dynamic_pointer_cast<Impl>(interface_ptr))
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

void RenderPass::Init(const Pattern& render_pattern, const Settings& settings)
{
    m_impl_ptr = std::dynamic_pointer_cast<Impl>(IRenderPass::Create(render_pattern.GetInterface(), settings));
    Transmitter<IObjectCallback>::Reset(m_impl_ptr.get());
    Transmitter<IRenderPassCallback>::Reset(m_impl_ptr.get());
}

void RenderPass::Release()
{
    Transmitter<IObjectCallback>::Reset();
    Transmitter<IRenderPassCallback>::Reset();
    m_impl_ptr.reset();
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

} // namespace Methane::Graphics::Rhi
