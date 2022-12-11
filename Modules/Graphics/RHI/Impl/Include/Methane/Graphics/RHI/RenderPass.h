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

    RenderPattern(const Ptr<IRenderPattern>& interface_ptr);
    RenderPattern(IRenderPattern& interface);
    RenderPattern(const RenderContext& render_context, const Settings& settings);

    void Init(const RenderContext& render_context, const Settings& settings);
    void Release();

    bool IsInitialized() const META_PIMPL_NOEXCEPT;
    IRenderPattern& GetInterface() const META_PIMPL_NOEXCEPT;

    bool SetName(const std::string& name) const;
    const std::string& GetName() const META_PIMPL_NOEXCEPT;

private:
    class Impl;

    UniquePtr<Impl> m_impl_ptr;
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

    RenderPass(const Ptr<IRenderPass>& interface_ptr);
    RenderPass(IRenderPass& interface);
    RenderPass(const Pattern& render_pattern, const Settings& settings);

    void Init(const Pattern& render_pattern, const Settings& settings);
    void Release();

    bool IsInitialized() const META_PIMPL_NOEXCEPT;
    IRenderPass& GetInterface() const META_PIMPL_NOEXCEPT;

    bool SetName(const std::string& name) const;
    const std::string& GetName() const META_PIMPL_NOEXCEPT;

private:
    class Impl;

    UniquePtr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi
