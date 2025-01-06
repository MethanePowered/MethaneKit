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

FILE: Methane/Graphics/RHI/IRenderPattern.h
Methane render pattern interface.

******************************************************************************/

#pragma once

#include "IObject.h"

#include <Methane/Memory.hpp>
#include <Methane/Data/IEmitter.h>
#include <Methane/Data/EnumMask.hpp>
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

    [[nodiscard]] friend bool operator==(const RenderPassAttachment& left, const RenderPassAttachment& right)
    {
        return std::tie(left.attachment_index, left.format, left.samples_count, left.load_action, left.store_action) ==
               std::tie(right.attachment_index, right.format, right.samples_count, right.load_action, right.store_action);
    }

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

    [[nodiscard]] friend bool operator==(const RenderPassColorAttachment& left, const RenderPassColorAttachment& right)
    {
        return static_cast<const RenderPassAttachment&>(left) == static_cast<const RenderPassAttachment&>(right) &&
               left.clear_color == right.clear_color;
    }

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

    [[nodiscard]] friend bool operator==(const RenderPassDepthAttachment& left, const RenderPassDepthAttachment& right)
    {
        return static_cast<const RenderPassAttachment&>(left) == static_cast<const RenderPassAttachment&>(right) &&
               left.clear_value == right.clear_value;
    }

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

    [[nodiscard]] friend bool operator==(const RenderPassStencilAttachment& left, const RenderPassStencilAttachment& right)
    {
        return static_cast<const RenderPassAttachment&>(left) == static_cast<const RenderPassAttachment&>(right) &&
               left.clear_value == right.clear_value;
    }

    [[nodiscard]] explicit operator std::string() const override;

    [[nodiscard]] Type GetType() const noexcept override { return Type::Stencil; }
};

enum class RenderPassAccess : uint32_t
{
    ShaderResources,
    Samplers,
    RenderTargets,
    DepthStencil
};

using RenderPassAccessMask = Data::EnumMask<RenderPassAccess>;

struct RenderPatternSettings
{
    RenderPassColorAttachments       color_attachments;
    Opt<RenderPassDepthAttachment>   depth_attachment;
    Opt<RenderPassStencilAttachment> stencil_attachment;
    RenderPassAccessMask             shader_access;
    bool                             is_final_pass = true;

    [[nodiscard]] friend bool operator==(const RenderPatternSettings& left, const RenderPatternSettings& right)
    {
        return std::tie(left.color_attachments, left.depth_attachment, left.stencil_attachment, left.shader_access, left.is_final_pass) ==
               std::tie(right.color_attachments, right.depth_attachment, right.stencil_attachment, right.shader_access, right.is_final_pass);
    }

    [[nodiscard]] explicit operator std::string() const;
};

struct IRenderContext;
struct IRenderPass;
struct RenderPassSettings;

struct IRenderPattern
    : virtual IObject // NOSONAR
{
    using Attachment        = RenderPassAttachment;
    using ColorAttachment   = RenderPassColorAttachment;
    using ColorAttachments  = RenderPassColorAttachments;
    using DepthAttachment   = RenderPassDepthAttachment;
    using StencilAttachment = RenderPassStencilAttachment;
    using AccessMask        = RenderPassAccessMask;
    using Access            = RenderPassAccess;
    using Settings          = RenderPatternSettings;

    // Create IRenderPattern instance
    [[nodiscard]] static Ptr<IRenderPattern> Create(IRenderContext& render_context, const Settings& settings);

    // IRenderPattern interface
    [[nodiscard]] virtual Ptr<IRenderPass>      CreateRenderPass(const RenderPassSettings& settings) = 0;
    [[nodiscard]] virtual const IRenderContext& GetRenderContext() const noexcept = 0;
    [[nodiscard]] virtual IRenderContext&       GetRenderContext() noexcept = 0;
    [[nodiscard]] virtual const Settings&       GetSettings() const noexcept = 0;
    [[nodiscard]] virtual Data::Size            GetAttachmentCount() const noexcept = 0;
    [[nodiscard]] virtual AttachmentFormats     GetAttachmentFormats() const noexcept = 0;
};

} // namespace Methane::Graphics::Rhi
