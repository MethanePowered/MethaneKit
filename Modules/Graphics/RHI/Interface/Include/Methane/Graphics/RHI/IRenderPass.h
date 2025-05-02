/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/IRenderPass.h
Methane render pass interface: specifies output texture views of the render pattern.

******************************************************************************/

#pragma once

#include "IRenderPattern.h"
#include "ITexture.h"

namespace Methane::Graphics::Rhi
{

struct IRenderPass;

struct IRenderPassCallback
{
    virtual void OnRenderPassUpdated(const IRenderPass& render_pass) = 0;

    virtual ~IRenderPassCallback() = default;
};

struct RenderPassSettings
{
    TextureViews attachments;
    FrameSize    frame_size;

    [[nodiscard]] friend bool operator==(const RenderPassSettings& left, const RenderPassSettings& right) = default;
};

struct IRenderPass
    : virtual IObject // NOSONAR
    , virtual Data::IEmitter<IRenderPassCallback> // NOSONAR
{
    using IPattern          = IRenderPattern;
    using Attachment        = RenderPassAttachment;
    using ColorAttachment   = RenderPassColorAttachment;
    using ColorAttachments  = RenderPassColorAttachments;
    using DepthAttachment   = RenderPassDepthAttachment;
    using StencilAttachment = RenderPassStencilAttachment;
    using AccessMask        = RenderPassAccessMask;
    using Access            = RenderPassAccess;
    using Settings          = RenderPassSettings;
    using ICallback         = IRenderPassCallback;

    // Create IRenderPass instance
    [[nodiscard]] static Ptr<IRenderPass> Create(IRenderPattern& render_pattern, const Settings& settings);

    // IRenderPass interface
    virtual const IRenderPattern& GetPattern() const noexcept = 0;
    virtual IRenderPattern& GetPattern() noexcept = 0;
    virtual const Settings& GetSettings() const noexcept = 0;
    virtual bool Update(const Settings& settings) = 0;
    virtual void ReleaseAttachmentTextures() = 0;
};

} // namespace Methane::Graphics::Rhi
