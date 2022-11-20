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
Methane render pass interface: specifies output of the graphics pipeline.

******************************************************************************/

#pragma once

#include "ITexture.h"
#include "IObject.h"

#include <Methane/Memory.hpp>
#include <Methane/Data/IEmitter.h>
#include <Methane/Graphics/Types.h>
#include <Methane/Graphics/Color.hpp>

#include <vector>

namespace Methane::Graphics::Rhi
{

struct RenderPassAttachment
{
    enum class Type : uint32_t
    {
        Color,
        Depth,
        Stencil
    };

    enum class LoadAction : uint32_t
    {
        DontCare = 0U,
        Load,
        Clear,
    };

    enum class StoreAction : uint32_t
    {
        DontCare = 0U,
        Store,
        Resolve,
    };

    Data::Index attachment_index = 0U;
    PixelFormat format           = PixelFormat::Unknown;
    Data::Size  samples_count    = 1U;
    LoadAction  load_action      = LoadAction::DontCare;
    StoreAction store_action     = StoreAction::DontCare;

    RenderPassAttachment() = default;
    RenderPassAttachment(Data::Index attachment_index,
                         PixelFormat format,
                         Data::Size  samples_count,
                         LoadAction  load_action  = LoadAction::DontCare,
                         StoreAction store_action = StoreAction::DontCare);
    virtual ~RenderPassAttachment() = default;

    [[nodiscard]] bool operator==(const RenderPassAttachment& other) const;
    [[nodiscard]] bool operator!=(const RenderPassAttachment& other) const;
    [[nodiscard]] virtual explicit operator std::string() const;
    [[nodiscard]] virtual Type GetType() const noexcept = 0;
};

struct RenderPassColorAttachment : RenderPassAttachment
{
    Color4F clear_color;

    RenderPassColorAttachment(Data::Index    attachment_index,
                              PixelFormat    format,
                              Data::Size     samples_count,
                              LoadAction     load_action  = LoadAction::DontCare,
                              StoreAction    store_action = StoreAction::DontCare,
                              const Color4F& clear_color  = Color4F());

    [[nodiscard]] bool operator==(const RenderPassColorAttachment& other) const;
    [[nodiscard]] bool operator!=(const RenderPassColorAttachment& other) const;
    [[nodiscard]] explicit operator std::string() const override;

    [[nodiscard]] Type GetType() const noexcept override { return Type::Color; }
};

using RenderPassColorAttachments = std::vector<RenderPassColorAttachment>;

struct RenderPassDepthAttachment final : RenderPassAttachment
{
    Depth clear_value = 1.F;

    RenderPassDepthAttachment() = default;
    RenderPassDepthAttachment(Data::Index attachment_index,
                              PixelFormat format,
                              Data::Size  samples_count,
                              LoadAction  load_action  = LoadAction::DontCare,
                              StoreAction store_action = StoreAction::DontCare,
                              Depth       clear_value  = 1.F);

    [[nodiscard]] bool operator==(const RenderPassDepthAttachment& other) const;
    [[nodiscard]] bool operator!=(const RenderPassDepthAttachment& other) const;
    [[nodiscard]] explicit operator std::string() const override;

    [[nodiscard]] Type GetType() const noexcept override { return Type::Depth; }
};

struct RenderPassStencilAttachment final : RenderPassAttachment
{
    Stencil clear_value = 0U;

    RenderPassStencilAttachment() = default;
    RenderPassStencilAttachment(Data::Index attachment_index,
                                PixelFormat format,
                                Data::Size  samples_count,
                                LoadAction  load_action  = LoadAction::DontCare,
                                StoreAction store_action = StoreAction::DontCare,
                                Stencil     clear_value  = 0U);

    [[nodiscard]] bool operator==(const RenderPassStencilAttachment& other) const;
    [[nodiscard]] bool operator!=(const RenderPassStencilAttachment& other) const;
    [[nodiscard]] explicit operator std::string() const override;

    [[nodiscard]] Type GetType() const noexcept override { return Type::Stencil; }
};

union RenderPassAccess
{
    enum class Bit : uint32_t
    {
        ShaderResources,
        Samplers,
        RenderTargets,
        DepthStencil
    };

    struct
    {
        bool shader_resources : 1;
        bool samplers         : 1;
        bool render_targets   : 1;
        bool depth_stencil    : 1;
    };

    uint32_t mask;

    RenderPassAccess() noexcept;
    explicit RenderPassAccess(uint32_t mask) noexcept;
    explicit RenderPassAccess(const std::initializer_list<Bit>& bits);

    void SetBit(Bit bit, bool value);
    std::vector<Bit> GetBits() const;
    std::vector<std::string> GetBitNames() const;
};

struct RenderPatternSettings
{
    RenderPassColorAttachments       color_attachments;
    Opt<RenderPassDepthAttachment>   depth_attachment;
    Opt<RenderPassStencilAttachment> stencil_attachment;
    RenderPassAccess                 shader_access;
    bool                             is_final_pass = true;

    [[nodiscard]] bool operator==(const RenderPatternSettings& other) const;
    [[nodiscard]] bool operator!=(const RenderPatternSettings& other) const;
    [[nodiscard]] explicit operator std::string() const;
};

struct IRenderContext;

struct IRenderPattern
    : virtual IObject // NOSONAR
{
    using Attachment        = RenderPassAttachment;
    using ColorAttachment   = RenderPassColorAttachment;
    using ColorAttachments  = RenderPassColorAttachments;
    using DepthAttachment   = RenderPassDepthAttachment;
    using StencilAttachment = RenderPassStencilAttachment;
    using Access            = RenderPassAccess;
    using Settings          = RenderPatternSettings;

    // Create IRenderPattern instance
    [[nodiscard]] static Ptr<IRenderPattern> Create(IRenderContext& render_context, const Settings& settings);

    // IRenderPattern interface
    [[nodiscard]] virtual const IRenderContext& GetRenderContext() const noexcept = 0;
    [[nodiscard]] virtual IRenderContext&       GetRenderContext() noexcept = 0;
    [[nodiscard]] virtual const Settings&       GetSettings() const noexcept = 0;
    [[nodiscard]] virtual Data::Size            GetAttachmentCount() const noexcept = 0;
    [[nodiscard]] virtual AttachmentFormats     GetAttachmentFormats() const noexcept = 0;
};

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

    [[nodiscard]] bool operator==(const RenderPassSettings& other) const;
    [[nodiscard]] bool operator!=(const RenderPassSettings& other) const;
};

struct IRenderPass
    : virtual IObject // NOSONAR
    , virtual Data::IEmitter<IRenderPassCallback> // NOSONAR
{
    using Pattern           = IRenderPattern;
    using Attachment        = RenderPassAttachment;
    using ColorAttachment   = RenderPassColorAttachment;
    using ColorAttachments  = RenderPassColorAttachments;
    using DepthAttachment   = RenderPassDepthAttachment;
    using StencilAttachment = RenderPassStencilAttachment;
    using Access            = RenderPassAccess;
    using Settings          = RenderPassSettings;
    using ICallback         = IRenderPassCallback;

    // Create IRenderPass instance
    [[nodiscard]] static Ptr<IRenderPass> Create(IRenderPattern& render_pattern, const Settings& settings);

    // IRenderPass interface
    virtual const IRenderPattern& GetPattern() const noexcept = 0;
    virtual const Settings& GetSettings() const noexcept = 0;
    virtual bool Update(const Settings& settings) = 0;
    virtual void ReleaseAttachmentTextures() = 0;
};

} // namespace Methane::Graphics::Rhi
