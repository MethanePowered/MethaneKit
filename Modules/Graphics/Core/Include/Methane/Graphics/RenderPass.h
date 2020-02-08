/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
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

#include "Types.h"
#include "Texture.h"

#include <Methane/Memory.hpp>

#include <vector>

namespace Methane::Graphics
{

struct RenderContext;

struct RenderPass
{
    struct Attachment
    {
        enum class LoadAction : uint32_t
        {
            DontCare = 0u,
            Load,
            Clear,
        };

        enum class StoreAction : uint32_t
        {
            DontCare = 0u,
            Store,
            Resolve,
        };
        
        WeakPtr<Texture> wp_texture;
        uint32_t         level        = 0u;
        uint32_t         slice        = 0u;
        uint32_t         depth_plane  = 0u;
        LoadAction       load_action  = LoadAction::DontCare;
        StoreAction      store_action = StoreAction::DontCare;
        
        bool operator==(const Attachment& other) const;
    };
    
    struct ColorAttachment : Attachment
    {
        Color4f clear_color;
        
        ColorAttachment(const Attachment&& attach, const Color4f& in_clear_color = Color4f()) : Attachment(attach), clear_color(in_clear_color) { }
        
        bool operator==(const ColorAttachment& other) const;
    };
    
    using ColorAttachments = std::vector<ColorAttachment>;
    
    struct DepthAttachment : Attachment
    {
        Depth clear_value = 1.f;
        
        DepthAttachment() = default;
        DepthAttachment(const Attachment&& attach, Depth in_clear_value = 1.f) : Attachment(attach), clear_value(in_clear_value) { }
        
        bool operator==(const DepthAttachment& other) const;
    };
    
    struct StencilAttachment : Attachment
    {
        Stencil clear_value = 0u;
        
        StencilAttachment() = default;
        StencilAttachment(const Attachment&& attach, Stencil in_clear_value = 0u) : Attachment(attach), clear_value(in_clear_value) { }
        
        bool operator==(const StencilAttachment& other) const;
    };

    struct Access
    {
        using Mask = uint32_t;
        enum Value : Mask
        {
            None            = 0u,
            ShaderResources = 1u << 0u,
            Samplers        = 1u << 1u,
            RenderTargets   = 1u << 2u,
            DepthStencil    = 1u << 3u,
            All             = ~0u,
        };

        using Values = std::array<Value, 4>;
        static constexpr Values values = { ShaderResources, Samplers, RenderTargets, DepthStencil };
    };

    struct Settings
    {
        ColorAttachments   color_attachments;
        DepthAttachment    depth_attachment;
        StencilAttachment  stencil_attachment;
        Access::Mask       shader_access_mask = Access::None;

        bool operator==(const Settings& other) const;
        bool operator!=(const Settings& other) const;
    };

    // Create RenderPass instance
    static Ptr<RenderPass> Create(RenderContext& context, const Settings& settings);

    // RenderPass interface
    virtual void  Update(const Settings& settings) = 0;
    virtual const Settings& GetSettings() const = 0;

    virtual ~RenderPass() = default;
};

} // namespace Methane::Graphics
