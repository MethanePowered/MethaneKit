/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

#include <memory>
#include <vector>

namespace Methane::Graphics
{

struct Context;
struct Resource;

struct RenderPass
{
    using Ptr = std::shared_ptr<RenderPass>;

    struct Attachment
    {
        enum class LoadAction : uint32_t
        {
            DontCare = 0,
            Load,
            Clear,
        };

        enum class StoreAction : uint32_t
        {
            DontCare = 0,
            Store,
            Unknown,
        };
        
        Texture::WeakPtr wp_texture;
        uint32_t         level        = 0;
        uint32_t         slice        = 0;
        uint32_t         depth_plane  = 0;
        LoadAction       load_action  = LoadAction::DontCare;
        StoreAction      store_action = StoreAction::Unknown;
        
        bool operator==(const Attachment& other) const;
    };
    
    struct ColorAttachment : Attachment
    {
        Color       clear_color;
        
        ColorAttachment(const Attachment&& attach, const Color& in_clear_color = Color()) : Attachment(attach), clear_color(in_clear_color) { }
        
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
        Stencil clear_value = 0;
        
        StencilAttachment() = default;
        StencilAttachment(const Attachment&& attach, Stencil in_clear_value = 0) : Attachment(attach), clear_value(in_clear_value) { }
        
        bool operator==(const StencilAttachment& other) const;
    };

    struct Access
    {
        using Mask = uint32_t;
        enum Value : Mask
        {
            None            = 0,
            ShaderResources = 1 << 0,
            Samplers        = 1 << 1,
            RenderTargets   = 1 << 2,
            DepthStencil    = 1 << 3,
            All             = static_cast<Mask>(~0),
        };

        using Values = std::array<Value, 4>;
        static constexpr Values values = { ShaderResources, Samplers, RenderTargets, DepthStencil };
    };

    struct Settings
    {
        using Ptr = std::unique_ptr<Settings>;
        
        ColorAttachments   color_attachments;
        DepthAttachment    depth_attachment;
        StencilAttachment  stencil_attachment;
        Access::Mask       shader_access_mask = Access::None;

        bool operator==(const Settings& other) const;
        bool operator!=(const Settings& other) const;
    };

    // Create RenderPass instance
    static Ptr Create(Context& context, const Settings& settings);

    // RenderPass interface
    virtual void  Update(const Settings& settings) = 0;
    virtual const Settings& GetSettings() const = 0;

    virtual ~RenderPass() = default;
};

} // namespace Methane::Graphics
