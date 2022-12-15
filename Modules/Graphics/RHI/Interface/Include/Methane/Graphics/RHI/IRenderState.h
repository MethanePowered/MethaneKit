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

FILE: Methane/Graphics/RHI/IRenderState.h
Methane render state interface: specifies configuration of the graphics pipeline.

******************************************************************************/

#pragma once

#include "IObject.h"
#include "IProgram.h"

#include <Methane/Graphics/Types.h>
#include <Methane/Graphics/Volume.hpp>
#include <Methane/Graphics/Color.hpp>
#include <Methane/Data/EnumMask.hpp>
#include <Methane/Memory.hpp>
#include <vector>

namespace Methane::Graphics::Rhi
{

enum class RasterizerCullMode : uint32_t
{
    None = 0,
    Back,
    Front,
};

enum class RasterizerFillMode : uint32_t
{
    Solid = 0,
    Wireframe,
};

struct RasterizerSettings
{
    using CullMode = RasterizerCullMode;
    using FillMode = RasterizerFillMode;

    bool     is_front_counter_clockwise = false;
    CullMode cull_mode                  = CullMode::Back;
    FillMode fill_mode                  = FillMode::Solid;
    uint32_t sample_count               = 1;
    bool     alpha_to_coverage_enabled  = false;

    [[nodiscard]] bool operator==(const RasterizerSettings& other) const noexcept;
    [[nodiscard]] bool operator!=(const RasterizerSettings& other) const noexcept;
    [[nodiscard]] explicit operator std::string() const;
};

enum class BlendingColorChannel : uint32_t
{
    Red,
    Green,
    Blue,
    Alpha
};

using BlendingColorChannelMask = Data::EnumMask<BlendingColorChannel>;

enum class BlendingOperation : uint32_t
{
    Add = 0U,
    Subtract,
    ReverseSubtract,
    Minimum,
    Maximum,
};

enum class BlendingFactor : uint32_t
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

struct RenderTargetSettings
{
    using ColorChannelMask = BlendingColorChannelMask;
    using ColorChannel     = BlendingColorChannel;
    using Operation        = BlendingOperation;
    using Factor           = BlendingFactor;

    bool             blend_enabled             = false;
    ColorChannelMask color_write               { ~0U };
    Operation        rgb_blend_op              = Operation::Add;
    Operation        alpha_blend_op            = Operation::Add;
    Factor           source_rgb_blend_factor   = Factor::One;
    Factor           source_alpha_blend_factor = Factor::One;
    Factor           dest_rgb_blend_factor     = Factor::Zero;
    Factor           dest_alpha_blend_factor   = Factor::Zero;

    bool operator==(const RenderTargetSettings& other) const noexcept;
    bool operator!=(const RenderTargetSettings& other) const noexcept;
    [[nodiscard]] explicit operator std::string() const;
};

struct BlendingSettings
{
    using ColorChannelMask = BlendingColorChannelMask;
    using ColorChannel     = BlendingColorChannel;
    using Operation        = BlendingOperation;
    using Factor           = BlendingFactor;
    using RenderTarget     = RenderTargetSettings;
    using RenderTargets    = std::array<RenderTarget, 8>;

    // NOTE: If is_independent set to false, only the render_targets[0] members are used
    bool          is_independent = false;
    RenderTargets render_targets;

    [[nodiscard]] bool operator==(const BlendingSettings& other) const noexcept;
    [[nodiscard]] bool operator!=(const BlendingSettings& other) const noexcept;
    [[nodiscard]] explicit operator std::string() const;
};

struct DepthSettings
{
    bool    enabled         = false;
    bool    write_enabled   = true;
    Compare compare         = Compare::Less;

    [[nodiscard]] bool operator==(const DepthSettings& other) const noexcept;
    [[nodiscard]] bool operator!=(const DepthSettings& other) const noexcept;
    [[nodiscard]] explicit operator std::string() const;
};

enum class FaceOperation : uint32_t
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
    using Operation = FaceOperation;

    Operation  stencil_failure    = Operation::Keep;
    Operation  stencil_pass       = Operation::Keep; // DX, VK only
    Operation  depth_failure      = Operation::Keep;
    Operation  depth_stencil_pass = Operation::Keep; // Metal only
    Compare    compare            = Compare::Always;

    [[nodiscard]] bool operator==(const FaceOperations& other) const noexcept;
    [[nodiscard]] bool operator!=(const FaceOperations& other) const noexcept;
    [[nodiscard]] explicit operator std::string() const;
};

struct StencilSettings
{
    bool           enabled    = false;
    uint8_t        read_mask  = static_cast<uint8_t>(~0x0);
    uint8_t        write_mask = static_cast<uint8_t>(~0x0);
    FaceOperations front_face;
    FaceOperations back_face;

    [[nodiscard]] bool operator==(const StencilSettings& other) const noexcept;
    [[nodiscard]] bool operator!=(const StencilSettings& other) const noexcept;
    [[nodiscard]] explicit operator std::string() const;
};

enum class RenderStateGroup : uint32_t
{
    Program,
    Rasterizer,
    Blending,
    BlendingColor,
    DepthStencil,
};

using RenderStateGroupMask = Data::EnumMask<RenderStateGroup>;

struct IRenderContext;
struct IProgram;
struct IRenderPattern;

struct RenderStateSettings
{
    using GroupMask = RenderStateGroupMask;
    using Group     = RenderStateGroup;

    // NOTE: members are ordered by the usage frequency,
    //       for convenient setup with initializer lists
    //       (default states may be skipped at initialization)
    Ptr<IProgram>       program_ptr;
    Ptr<IRenderPattern> render_pattern_ptr;
    RasterizerSettings  rasterizer;
    DepthSettings       depth;
    StencilSettings     stencil;
    BlendingSettings    blending;
    Color4F             blending_color;

    [[nodiscard]] static GroupMask Compare(const RenderStateSettings& left, const RenderStateSettings& right, GroupMask compare_groups = GroupMask(~0U)) noexcept;
    [[nodiscard]] bool operator==(const RenderStateSettings& other) const noexcept;
    [[nodiscard]] bool operator!=(const RenderStateSettings& other) const noexcept;
    [[nodiscard]] explicit operator std::string() const;
};

struct IRenderState
    : virtual IObject // NOSONAR
{
public:
    using Rasterizer = RasterizerSettings;
    using Blending   = BlendingSettings;
    using Depth      = DepthSettings;
    using Stencil    = StencilSettings;
    using Groups     = RenderStateGroupMask;
    using Group      = RenderStateGroup;
    using Settings   = RenderStateSettings;

    // Create IRenderState instance
    [[nodiscard]] static Ptr<IRenderState> Create(const IRenderContext& context, const Settings& state_settings);

    // IRenderState interface
    [[nodiscard]] virtual const Settings& GetSettings() const noexcept = 0;
    virtual void Reset(const Settings& settings) = 0;
};

} // namespace Methane::Graphics::Rhi
