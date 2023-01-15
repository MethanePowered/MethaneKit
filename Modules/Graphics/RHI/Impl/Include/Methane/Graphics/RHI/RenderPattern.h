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

FILE: Methane/Graphics/RHI/RenderPattern.h
Methane RenderPattern PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include "Pimpl.h"

#include <Methane/Graphics/RHI/IRenderPattern.h>

namespace Methane::Graphics::META_GFX_NAME
{
class RenderPattern;
}

namespace Methane::Graphics::Rhi
{

class RenderContext;
class RenderPass;

struct RenderPassSettings;

class RenderPattern // NOSONAR - constructors and assignment operators are required to use forward declared Impl and Ptr<Impl> in header
{
public:
    using Attachment        = RenderPassAttachment;
    using ColorAttachment   = RenderPassColorAttachment;
    using ColorAttachments  = RenderPassColorAttachments;
    using DepthAttachment   = RenderPassDepthAttachment;
    using StencilAttachment = RenderPassStencilAttachment;
    using AccessMask        = RenderPassAccessMask;
    using Access            = RenderPassAccess;
    using Settings          = RenderPatternSettings;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(RenderPattern);
    META_PIMPL_METHODS_COMPARE_DECLARE(RenderPattern);

    META_RHI_API explicit RenderPattern(const Ptr<IRenderPattern>& interface_ptr);
    META_RHI_API explicit RenderPattern(IRenderPattern& interface_ref);
    META_RHI_API RenderPattern(const RenderContext& render_context, const Settings& settings);

    META_RHI_API bool IsInitialized() const META_PIMPL_NOEXCEPT;
    META_RHI_API IRenderPattern& GetInterface() const META_PIMPL_NOEXCEPT;
    META_RHI_API Ptr<IRenderPattern> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    META_RHI_API bool SetName(std::string_view name) const;
    META_RHI_API std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // Data::IEmitter<IObjectCallback> interface methods
    META_RHI_API void Connect(Data::Receiver<IObjectCallback>& receiver) const;
    META_RHI_API void Disconnect(Data::Receiver<IObjectCallback>& receiver) const;

    // IRenderPattern interface methods
    [[nodiscard]] META_RHI_API RenderPass        CreateRenderPass(const RenderPassSettings& settings) const;
    [[nodiscard]] META_RHI_API RenderContext     GetRenderContext() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API const Settings&   GetSettings() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API Data::Size        GetAttachmentCount() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API AttachmentFormats GetAttachmentFormats() const META_PIMPL_NOEXCEPT;

private:
    using Impl = Methane::Graphics::META_GFX_NAME::RenderPattern;

    Ptr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi

#ifdef META_RHI_PIMPL_INLINE

#include <Methane/Graphics/RHI/RenderPattern.cpp>

#endif // META_RHI_PIMPL_INLINE
