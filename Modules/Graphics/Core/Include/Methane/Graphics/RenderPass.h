/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RenderPass.h
Methane render pass interface: specifies output of the graphics pipeline.

******************************************************************************/

#pragma once

#include "Texture.h"
#include "Object.h"

#include <Methane/Memory.hpp>
#include <Methane/Graphics/Types.h>
#include <Methane/Graphics/Color.hpp>

#include <vector>

namespace Methane::Graphics
{

struct RenderContext;

struct RenderPass : virtual Object
{
    struct Attachment
    {
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
        
        Texture::Location  texture_location;
        LoadAction         load_action  = LoadAction::DontCare;
        StoreAction        store_action = StoreAction::DontCare;

        Attachment() = default;
        Attachment(Texture::Location&& texture_location,
                   LoadAction          load_action  = LoadAction::DontCare,
                   StoreAction         store_action = StoreAction::DontCare);
        virtual ~Attachment() = default;

        [[nodiscard]] bool operator==(const Attachment& other) const;
        [[nodiscard]] bool operator!=(const Attachment& other) const;
        [[nodiscard]] virtual explicit operator std::string() const;
    };
    
    struct ColorAttachment : Attachment
    {
        Color4F clear_color;
        
        ColorAttachment(Texture::Location&& texture_location,
                        LoadAction          load_action  = LoadAction::DontCare,
                        StoreAction         store_action = StoreAction::DontCare,
                        const Color4F&      clear_color  = Color4F());

        [[nodiscard]] bool operator==(const ColorAttachment& other) const;
        [[nodiscard]] bool operator!=(const ColorAttachment& other) const;
        [[nodiscard]] explicit operator std::string() const final;
    };
    
    using ColorAttachments = std::vector<ColorAttachment>;
    
    struct DepthAttachment : Attachment
    {
        Depth clear_value = 1.F;
        
        DepthAttachment() = default;
        DepthAttachment(Texture::Location&& texture_location,
                        LoadAction          load_action  = LoadAction::DontCare,
                        StoreAction         store_action = StoreAction::DontCare,
                        Depth               clear_value  = 1.F);

        [[nodiscard]] bool operator==(const DepthAttachment& other) const;
        [[nodiscard]] bool operator!=(const DepthAttachment& other) const;
        [[nodiscard]] explicit operator std::string() const final;
    };
    
    struct StencilAttachment : Attachment
    {
        Stencil clear_value = 0U;
        
        StencilAttachment() = default;
        StencilAttachment(Texture::Location&& texture_location,
                          LoadAction          load_action  = LoadAction::DontCare,
                          StoreAction         store_action = StoreAction::DontCare,
                          Stencil             clear_value  = 0U);

        [[nodiscard]] bool operator==(const StencilAttachment& other) const;
        [[nodiscard]] bool operator!=(const StencilAttachment& other) const;
        [[nodiscard]] explicit operator std::string() const final;
    };

    enum class Access : uint32_t
    {
        None            = 0U,
        ShaderResources = 1U << 0U,
        Samplers        = 1U << 1U,
        RenderTargets   = 1U << 2U,
        DepthStencil    = 1U << 3U,
        All             = ~0U,
    };

    struct Settings
    {
        ColorAttachments   color_attachments;
        DepthAttachment    depth_attachment;
        StencilAttachment  stencil_attachment;
        Access             shader_access_mask = Access::None;
        bool               is_final_pass = true;

        [[nodiscard]] bool operator==(const Settings& other) const;
        [[nodiscard]] bool operator!=(const Settings& other) const;
        [[nodiscard]] explicit operator std::string() const;
    };

    // Create RenderPass instance
    [[nodiscard]] static Ptr<RenderPass> Create(RenderContext& context, const Settings& settings);

    // RenderPass interface
    [[nodiscard]] virtual const Settings& GetSettings() const = 0;
    virtual bool Update(const Settings& settings) = 0;
    virtual void ReleaseAttachmentTextures() = 0;
};

} // namespace Methane::Graphics
