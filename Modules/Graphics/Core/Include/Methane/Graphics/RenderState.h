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

FILE: Methane/Graphics/RenderState.h
Methane render state interface: specifies configuration of the graphics pipeline.

******************************************************************************/

#pragma once

#include "Object.h"
#include "Program.h"

#include <Methane/Memory.hpp>
#include <Methane/Graphics/Types.h>
#include <Methane/Graphics/Volume.hpp>
#include <Methane/Graphics/Color.hpp>

namespace Methane::Graphics
{

struct RenderContext;
struct RenderCommandList;

struct ViewState
{
    struct Settings
    {
        Viewports    viewports;
        ScissorRects scissor_rects;

        [[nodiscard]] bool operator==(const Settings& other) const noexcept { return viewports == other.viewports && scissor_rects == other.scissor_rects; }
        [[nodiscard]] bool operator!=(const Settings& other) const noexcept { return !operator==(other); }
        [[nodiscard]] explicit operator std::string() const;
    };

    // Create ViewState instance
    [[nodiscard]] static Ptr<ViewState> Create(const Settings& state_settings);

    // ViewState interface
    [[nodiscard]] virtual const Settings& GetSettings() const noexcept = 0;
    virtual bool Reset(const Settings& settings) = 0;
    virtual bool SetViewports(const Viewports& viewports) = 0;
    virtual bool SetScissorRects(const ScissorRects& scissor_rects) = 0;

    virtual ~ViewState() = default;
};

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

        [[nodiscard]] bool operator==(const Rasterizer& other) const noexcept;
        [[nodiscard]] bool operator!=(const Rasterizer& other) const noexcept;
        [[nodiscard]] explicit operator std::string() const;
    };

    struct Blending
    {
        enum class ColorChannels : uint32_t
        {
            None    = 0U,
            Red     = 1U << 0U,
            Green   = 1U << 1U,
            Blue    = 1U << 2U,
            Alpha   = 1U << 3U,
            All     = ~0U
        };

        enum class Operation : uint32_t
        {
            Add = 0U,
            Subtract,
            ReverseSubtract,
            Minimum,
            Maximum,
        };

        enum class Factor : uint32_t
        {
            Zero = 0U,
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
            bool          blend_enabled             = false;
            ColorChannels write_mask                = ColorChannels::All;
            Operation     rgb_blend_op              = Operation::Add;
            Operation     alpha_blend_op            = Operation::Add;
            Factor        source_rgb_blend_factor   = Factor::One;
            Factor        source_alpha_blend_factor = Factor::One;
            Factor        dest_rgb_blend_factor     = Factor::Zero;
            Factor        dest_alpha_blend_factor   = Factor::Zero;

            bool operator==(const RenderTarget& other) const noexcept;
            bool operator!=(const RenderTarget& other) const noexcept;
            [[nodiscard]] explicit operator std::string() const;
        };

        // NOTE: If is_independent set to false, only the render_targets[0] members are used
        bool                        is_independent = false;
        std::array<RenderTarget, 8> render_targets;

        [[nodiscard]] bool operator==(const Blending& other) const noexcept;
        [[nodiscard]] bool operator!=(const Blending& other) const noexcept;
        [[nodiscard]] explicit operator std::string() const;
    };
    
    struct Depth
    {
        bool    enabled         = false;
        bool    write_enabled   = true;
        Compare compare         = Compare::Less;

        [[nodiscard]] bool operator==(const Depth& other) const noexcept;
        [[nodiscard]] bool operator!=(const Depth& other) const noexcept;
        [[nodiscard]] explicit operator std::string() const;
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

            [[nodiscard]] bool operator==(const FaceOperations& other) const noexcept;
            [[nodiscard]] bool operator!=(const FaceOperations& other) const noexcept;
            [[nodiscard]] explicit operator std::string() const;
        };
        
        bool           enabled           = false;
        uint8_t        read_mask         = static_cast<uint8_t>(~0x0);
        uint8_t        write_mask        = static_cast<uint8_t>(~0x0);
        FaceOperations front_face;
        FaceOperations back_face;

        [[nodiscard]] bool operator==(const Stencil& other) const noexcept;
        [[nodiscard]] bool operator!=(const Stencil& other) const noexcept;
        [[nodiscard]] explicit operator std::string() const;
    };

    enum class Groups : uint32_t
    {
        None          = 0U,
        Program       = 1U << 0U,
        Rasterizer    = 1U << 1U,
        Blending      = 1U << 2U,
        BlendingColor = 1U << 3U,
        DepthStencil  = 1U << 4U,
        All           = ~0U
    };

    struct Settings
    {
        // NOTE: members are ordered by the usage frequency,
        //       for convenient setup with initializer lists
        //       (default states may be skipped at initialization)
        Ptr<Program> program_ptr;
        Rasterizer   rasterizer;
        Depth        depth;
        Stencil      stencil;
        Blending     blending;
        Color4F      blending_color;

        [[nodiscard]] static Groups Compare(const Settings& left, const Settings& right, Groups compare_groups = Groups::All) noexcept;
        [[nodiscard]] bool operator==(const Settings& other) const noexcept;
        [[nodiscard]] bool operator!=(const Settings& other) const noexcept;
        [[nodiscard]] explicit operator std::string() const;
    };

    // Create RenderState instance
    [[nodiscard]] static Ptr<RenderState> Create(const RenderContext& context, const Settings& state_settings);

    // RenderState interface
    [[nodiscard]] virtual const Settings& GetSettings() const noexcept = 0;
    virtual void Reset(const Settings& settings) = 0;
};

} // namespace Methane::Graphics
