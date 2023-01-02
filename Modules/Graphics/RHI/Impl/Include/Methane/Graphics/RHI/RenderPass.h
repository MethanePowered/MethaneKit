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

FILE: Methane/Graphics/RHI/RenderPass.h
Methane RenderPass PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include "Pimpl.h"

#include <Methane/Graphics/RHI/IRenderPass.h>

namespace Methane::Graphics::META_GFX_NAME
{
class RenderPass;
class RenderPattern;
}

namespace Methane::Graphics::Rhi
{

class RenderContext;

class RenderPattern
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

    META_RHI_API void Init(const RenderContext& render_context, const Settings& settings);
    META_RHI_API void Release();

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
    [[nodiscard]] META_RHI_API RenderContext     GetRenderContext() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API const Settings&   GetSettings() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API Data::Size        GetAttachmentCount() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API AttachmentFormats GetAttachmentFormats() const META_PIMPL_NOEXCEPT;

private:
    using Impl = Methane::Graphics::META_GFX_NAME::RenderPattern;

    META_RHI_API RenderPattern(Ptr<Impl>&& impl_ptr);

    Ptr<Impl> m_impl_ptr;
};

class RenderPass
{
public:
    using Pattern           = RenderPattern;
    using Attachment        = RenderPassAttachment;
    using ColorAttachment   = RenderPassColorAttachment;
    using ColorAttachments  = RenderPassColorAttachments;
    using DepthAttachment   = RenderPassDepthAttachment;
    using StencilAttachment = RenderPassStencilAttachment;
    using AccessMask        = RenderPassAccessMask;
    using Access            = RenderPassAccess;
    using Settings          = RenderPassSettings;
    using ICallback         = IRenderPassCallback;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(RenderPass);
    META_PIMPL_METHODS_COMPARE_DECLARE(RenderPass);

    META_RHI_API explicit RenderPass(const Ptr<IRenderPass>& interface_ptr);
    META_RHI_API explicit RenderPass(IRenderPass& interface_ref);
    META_RHI_API RenderPass(const Pattern& render_pattern, const Settings& settings);

    META_RHI_API void Init(const Pattern& render_pattern, const Settings& settings);
    META_RHI_API void Release();

    META_RHI_API bool IsInitialized() const META_PIMPL_NOEXCEPT;
    META_RHI_API IRenderPass& GetInterface() const META_PIMPL_NOEXCEPT;
    META_RHI_API Ptr<IRenderPass> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    META_RHI_API bool SetName(std::string_view name) const;
    META_RHI_API std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // Data::IEmitter<IObjectCallback> interface methods
    META_RHI_API void Connect(Data::Receiver<IObjectCallback>& receiver) const;
    META_RHI_API void Disconnect(Data::Receiver<IObjectCallback>& receiver) const;

    // IRenderPass interface methods
    META_RHI_API RenderPattern GetPattern() const META_PIMPL_NOEXCEPT;
    META_RHI_API const Settings& GetSettings() const META_PIMPL_NOEXCEPT;
    META_RHI_API bool Update(const Settings& settings) const META_PIMPL_NOEXCEPT;
    META_RHI_API void ReleaseAttachmentTextures() const META_PIMPL_NOEXCEPT;

    // Data::IEmitter<IRenderPassCallback> interface methods
    META_RHI_API void Connect(Data::Receiver<IRenderPassCallback>& receiver) const;
    META_RHI_API void Disconnect(Data::Receiver<IRenderPassCallback>& receiver) const;

private:
    using Impl = Methane::Graphics::META_GFX_NAME::RenderPass;

    META_RHI_API RenderPass(Ptr<Impl>&& impl_ptr);

    Ptr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi

#ifdef META_RHI_PIMPL_INLINE

#include <Methane/Graphics/RHI/RenderPass.cpp>

#endif // META_RHI_PIMPL_INLINE
