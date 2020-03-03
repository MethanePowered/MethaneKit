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

FILE: Methane/Graphics/RenderState.h
Methane render state interface: specifies configuration of the graphics pipeline.

******************************************************************************/

#pragma once

#include "Object.h"
#include "Program.h"
#include "Types.h"

#include <Methane/Memory.hpp>

namespace Methane::Graphics
{

struct RenderContext;
struct RenderCommandList;

struct RenderState : virtual Object
{
public:
    struct Rasterizer
    {
        enum class CullMode : uint32_t
        {
            None = 0,
            Back,
            Front,
        };
        
        enum class FillMode : uint32_t
        {
            Solid = 0,
            Wireframe,
        };
        
        bool     is_front_counter_clockwise = false;
        CullMode cull_mode                  = CullMode::Back;
        FillMode fill_mode                  = FillMode::Solid;
        uint32_t sample_count               = 1;
        bool     alpha_to_coverage_enabled  = false;

        bool operator==(const Rasterizer& other) const noexcept;
        bool operator!=(const Rasterizer& other) const noexcept;
    };

    struct Blending
    {
        struct ColorChannel
        {
            using Mask = uint32_t;
            enum Value : Mask
            {
                None    = 0u,
                Red     = 1u << 0u,
                Green   = 1u << 1u,
                Blue    = 1u << 2u,
                Alpha   = 1u << 3u,
                All     = ~0u,
            };

            ColorChannel() = delete;
        };

        enum class Operation : uint32_t
        {
            Add = 0u,
            Subtract,
            ReverseSubtract,
            Minimum,
            Maximum,
        };

        enum class Factor : uint32_t
        {
            Zero = 0u,
            One,
            SourceColor,
            OneMinusSourceColor,
            SourceAlpha,
            OneMinusSourceAlpha,
            DestinationColor,
            OneMinusDestinationColor,
            DestinationAlpha,
            OneMinusDestinationAlpha,
            SourceAlphaSaturated,
            BlendColor,
            OneMinusBlendColor,
            BlendAlpha,
            OneMinusBlendAlpha,
            Source1Color,
            OneMinusSource1Color,
            Source1Alpha,
            OneMinusSource1Alpha
        };

        struct RenderTarget
        {
            bool               blend_enabled             = false;
            ColorChannel::Mask write_mask                = ColorChannel::All;
            Operation          rgb_blend_op              = Operation::Add;
            Operation          alpha_blend_op            = Operation::Add;
            Factor             source_rgb_blend_factor   = Factor::One;
            Factor             source_alpha_blend_factor = Factor::One;
            Factor             dest_rgb_blend_factor     = Factor::Zero;
            Factor             dest_alpha_blend_factor   = Factor::Zero;

            bool operator==(const RenderTarget& other) const noexcept;
            bool operator!=(const RenderTarget& other) const noexcept;
        };

        // NOTE: If is_independent set to false, only the render_targets[0] members are used
        bool                        is_independent = false;
        std::array<RenderTarget, 8> render_targets;

        bool operator==(const Blending& other) const noexcept;
        bool operator!=(const Blending& other) const noexcept;
    };
    
    struct Depth
    {
        bool    enabled         = false;
        bool    write_enabled   = true;
        Compare compare         = Compare::Less;

        bool operator==(const Depth& other) const noexcept;
        bool operator!=(const Depth& other) const noexcept;
    };
    
    struct Stencil
    {
        enum class Operation : uint32_t
        {
            Keep = 0,
            Zero,
            Replace,
            Invert,
            IncrementClamp,
            DecrementClamp,
            IncrementWrap,
            DecrementWrap,
        };

        struct FaceOperations
        {
            Operation  stencil_failure   = Operation::Keep;
            Operation  stencil_pass      = Operation::Keep; // DX only
            Operation  depth_failure     = Operation::Keep;
            Operation  depth_stencil_pass= Operation::Keep; // Metal only
            Compare    compare           = Compare::Always;

            bool operator==(const FaceOperations& other) const noexcept;
            bool operator!=(const FaceOperations& other) const noexcept;
        };
        
        bool           enabled           = false;
        uint32_t       read_mask         = static_cast<uint32_t>(~0x0);
        uint32_t       write_mask        = static_cast<uint32_t>(~0x0);
        FaceOperations front_face;
        FaceOperations back_face;

        bool operator==(const Stencil& other) const noexcept;
        bool operator!=(const Stencil& other) const noexcept;
    };

    struct Group
    {
        using Mask = uint32_t;
        enum Value : Mask
        {
            None                = 0u,
            Program             = 1u << 0u,
            Rasterizer          = 1u << 1u,
            Blending            = 1u << 2u,
            BlendingColor       = 1u << 3u,
            DepthStencil        = 1u << 4u,
            Viewports           = 1u << 5u,
            ScissorRects        = 1u << 6u,
            All                 = ~0u
        };

        Group() = delete;
    };

    struct Settings
    {
        // NOTE: members are ordered by the usage frequency,
        //       for convenient setup with initializer lists
        //       (default states may be skipped at initialization)
        Ptr<Program> sp_program;
        Viewports    viewports;
        ScissorRects scissor_rects;
        Rasterizer   rasterizer;
        Depth        depth;
        Stencil      stencil;
        Blending     blending;
        Color4f      blending_color;

        static Group::Mask Compare(const Settings& left, const Settings& right, Group::Mask compare_groups = Group::All) noexcept;
    };

    // Create RenderState instance
    static Ptr<RenderState> Create(RenderContext& context, const Settings& state_settings);

    // RenderState interface
    virtual const Settings& GetSettings() const = 0;
    virtual void Reset(const Settings& settings) = 0;
    virtual void SetViewports(const Viewports& viewports) = 0;
    virtual void SetScissorRects(const ScissorRects& scissor_rects) = 0;

    virtual ~RenderState() = default;
};

} // namespace Methane::Graphics
