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

FILE: Methane/Graphics/RHI/IRenderState.cpp
Methane render state interface: specifies configuration of the graphics pipeline.

******************************************************************************/

#include <Methane/Graphics/RHI/IRenderState.h>

#include <Methane/Graphics/TypeFormatters.hpp>
#include <Methane/Instrumentation.h>

#include <fmt/format.h>
#include <fmt/ranges.h>

template<>
struct fmt::formatter<Methane::Graphics::Rhi::RenderTargetSettings>
{
    template<typename FormatContext>
    [[nodiscard]] auto format(const Methane::Graphics::Rhi::RenderTargetSettings& rt, FormatContext& ctx)
    { return format_to(ctx.out(), "{}", static_cast<std::string>(rt)); }

    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }
};

namespace Methane::Graphics::Rhi
{

bool ViewSettings::operator==(const ViewSettings& other) const noexcept
{
    META_FUNCTION_TASK();
    return viewports == other.viewports &&
           scissor_rects == other.scissor_rects;
}

bool ViewSettings::operator!=(const ViewSettings& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

ViewSettings::operator std::string() const
{
    META_FUNCTION_TASK();
    return fmt::format("  - Viewports: {};\n  - Scissor Rects: {}.", fmt::join(viewports, ", "), fmt::join(scissor_rects, ", "));
}

bool RasterizerSettings::operator==(const RasterizerSettings& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(is_front_counter_clockwise, cull_mode, fill_mode, sample_count, alpha_to_coverage_enabled) ==
           std::tie(other.is_front_counter_clockwise, other.cull_mode, other.fill_mode, other.sample_count, other.alpha_to_coverage_enabled);
}

bool RasterizerSettings::operator!=(const RasterizerSettings& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

RasterizerSettings::operator std::string() const
{
    META_FUNCTION_TASK();
    return fmt::format("  - Rasterizer: front={}, cull={}, fill={}, sample_count={}, alpha_to_coverage={}",
                       (is_front_counter_clockwise ? "CW" : "CCW"),
                       magic_enum::enum_name(cull_mode), magic_enum::enum_name(fill_mode),
                       sample_count, alpha_to_coverage_enabled);
}

BlendingColorChannels::BlendingColorChannels() noexcept
    : mask(0U)
{
}

BlendingColorChannels::BlendingColorChannels(uint32_t mask) noexcept
    : mask(mask)
{
}

BlendingColorChannels::BlendingColorChannels(const std::initializer_list<Bit>& bits)
    : mask(0U)
{
    META_FUNCTION_TASK();
    for(Bit bit : bits)
    {
        SetBit(bit, true);
    }
}

void BlendingColorChannels::SetBit(Bit bit, bool value)
{
    META_FUNCTION_TASK();
    switch(bit)
    {
    case Bit::Red:   red   = value; break;
    case Bit::Green: green = value; break;
    case Bit::Blue:  blue  = value; break;
    case Bit::Alpha: alpha = value; break;
    default: META_UNEXPECTED_ARG(bit);
    }
}

std::vector<BlendingColorChannels::Bit> BlendingColorChannels::GetBits() const
{
    META_FUNCTION_TASK();
    std::vector<Bit> bits;
    if (red)
        bits.push_back(Bit::Red);
    if (green)
        bits.push_back(Bit::Green);
    if (blue)
        bits.push_back(Bit::Blue);
    if (alpha)
        bits.push_back(Bit::Alpha);
    return bits;
}

std::vector<std::string> BlendingColorChannels::GetBitNames() const
{
    META_FUNCTION_TASK();
    const std::vector<Bit> bits = GetBits();
    std::vector<std::string> bit_names;
    for(Bit bit : bits)
    {
        bit_names.emplace_back(magic_enum::enum_name(bit));
    }
    return bit_names;
}

bool RenderTargetSettings::operator==(const RenderTargetSettings& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(blend_enabled, color_write.mask, rgb_blend_op, alpha_blend_op,
                    source_rgb_blend_factor, source_alpha_blend_factor,
                    dest_rgb_blend_factor, dest_alpha_blend_factor) ==
           std::tie(other.blend_enabled, other.color_write.mask, other.rgb_blend_op, other.alpha_blend_op,
                    other.source_rgb_blend_factor, other.source_alpha_blend_factor,
                    other.dest_rgb_blend_factor, other.dest_alpha_blend_factor);
}

bool RenderTargetSettings::operator!=(const RenderTargetSettings& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

RenderTargetSettings::operator std::string() const
{
    META_FUNCTION_TASK();
    if (!blend_enabled)
        return "    - Render Target blending is disabled";

    return fmt::format("    - Render Target blending: color_write={}, rgb_blend_op={}, alpha_blend_op={}, "
                       "source_rgb_blend_factor={}, source_alpha_blend_factor={}, dest_rgb_blend_factor={}, dest_alpha_blend_factor={}",
                       fmt::join(color_write.GetBitNames(), "|"),
                       magic_enum::enum_name(rgb_blend_op), magic_enum::enum_name(alpha_blend_op),
                       magic_enum::enum_name(source_rgb_blend_factor), magic_enum::enum_name(source_alpha_blend_factor),
                       magic_enum::enum_name(dest_rgb_blend_factor), magic_enum::enum_name(dest_alpha_blend_factor));
}

bool BlendingSettings::operator==(const BlendingSettings& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(is_independent, render_targets) ==
           std::tie(other.is_independent, other.render_targets);
}

bool BlendingSettings::operator!=(const BlendingSettings& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

BlendingSettings::operator std::string() const
{
    META_FUNCTION_TASK();
    return is_independent
           ? fmt::format("  - Blending is independent for render targets:\n{}.", fmt::join(render_targets, ";\n"))
           : fmt::format("  - Blending is common for all render targets:\n{}.", static_cast<std::string>(render_targets[0]));
}

bool DepthSettings::operator==(const DepthSettings& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(enabled, write_enabled, compare) ==
           std::tie(other.enabled, other.write_enabled, other.compare);
}

bool DepthSettings::operator!=(const DepthSettings& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

DepthSettings::operator std::string() const
{
    META_FUNCTION_TASK();
    if (!enabled)
        return "  - Depth is disabled";

    return fmt::format("  - Depth is enabled: write_enabled={}, compare={}",
                       write_enabled, magic_enum::enum_name(compare));
}

bool FaceOperations::operator==(const FaceOperations& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(stencil_failure, stencil_pass, depth_failure, depth_stencil_pass, compare) ==
           std::tie(other.stencil_failure, other.stencil_pass, other.depth_failure, other.depth_stencil_pass, other.compare);
}

bool FaceOperations::operator!=(const FaceOperations& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

FaceOperations::operator std::string() const
{
    META_FUNCTION_TASK();
    return fmt::format("face operations: stencil_failure={}, stencil_pass={}, depth_failure={}, depth_stencil_pass={}, compare={}",
                       magic_enum::enum_name(stencil_failure), magic_enum::enum_name(stencil_pass),
                       magic_enum::enum_name(depth_failure), magic_enum::enum_name(depth_stencil_pass),
                       magic_enum::enum_name(compare));
}

bool StencilSettings::operator==(const StencilSettings& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(enabled, read_mask, write_mask, front_face, back_face) ==
           std::tie(other.enabled, other.read_mask, other.write_mask, other.front_face, other.back_face);
}

bool StencilSettings::operator!=(const StencilSettings& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

StencilSettings::operator std::string() const
{
    META_FUNCTION_TASK();
    if (!enabled)
        return "  - Stencil is disabled";

    return fmt::format("  - Stencil is enabled: read_mask={:x}, color_write={:x}, face operations:\n    - Front {};\n    - Back {}.",
                       read_mask, write_mask, static_cast<std::string>(front_face), static_cast<std::string>(back_face));
}

RenderStateGroups::RenderStateGroups() noexcept
    : mask(0U)
{
}

RenderStateGroups::RenderStateGroups(uint32_t mask) noexcept
    : mask(mask)
{
}

RenderStateGroups::RenderStateGroups(const std::initializer_list<Bit>& bits)
    : mask(0U)
{
    META_FUNCTION_TASK();
    for(Bit bit : bits)
    {
        SetBit(bit, true);
    }
}

void RenderStateGroups::SetBit(Bit bit, bool value)
{
    META_FUNCTION_TASK();
    switch(bit)
    {
    case Bit::Program:       program        = value; break;
    case Bit::Rasterizer:    rasterizer     = value; break;
    case Bit::Blending:      blending       = value; break;
    case Bit::BlendingColor: blending_color = value; break;
    case Bit::DepthStencil:  depth_stencil  = value; break;
    default: META_UNEXPECTED_ARG(bit);
    }
}

std::vector<RenderStateGroups::Bit> RenderStateGroups::GetBits() const
{
    META_FUNCTION_TASK();
    std::vector<Bit> bits;
    if (program)
        bits.push_back(Bit::Program);
    if (rasterizer)
        bits.push_back(Bit::Rasterizer);
    if (blending)
        bits.push_back(Bit::Blending);
    if (blending_color)
        bits.push_back(Bit::BlendingColor);
    if (depth_stencil)
        bits.push_back(Bit::DepthStencil);
    return bits;
}

std::vector<std::string> RenderStateGroups::GetBitNames() const
{
    META_FUNCTION_TASK();
    const std::vector<Bit> bits = GetBits();
    std::vector<std::string> bit_names;
    for(Bit bit : bits)
    {
        bit_names.emplace_back(magic_enum::enum_name(bit));
    }
    return bit_names;
}

RenderStateGroups RenderStateSettings::Compare(const RenderStateSettings& left, const RenderStateSettings& right, Groups compare_groups) noexcept
{
    META_FUNCTION_TASK();
    Groups changed_state_groups;
    changed_state_groups.program        = compare_groups.program        && left.program_ptr.get() != right.program_ptr.get();
    changed_state_groups.rasterizer     = compare_groups.rasterizer     && left.rasterizer != right.rasterizer;
    changed_state_groups.blending       = compare_groups.blending       && left.blending != right.blending;
    changed_state_groups.blending_color = compare_groups.blending_color && left.blending_color != right.blending_color;
    changed_state_groups.depth_stencil  = compare_groups.depth_stencil  && (left.depth != right.depth || left.stencil != right.stencil);
    return changed_state_groups;
}

bool RenderStateSettings::operator==(const RenderStateSettings& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(program_ptr, rasterizer, depth, stencil, blending, blending_color) ==
           std::tie(other.program_ptr, other.rasterizer, other.depth, other.stencil, other.blending, other.blending_color);
}

bool RenderStateSettings::operator!=(const RenderStateSettings& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(program_ptr, rasterizer, depth, stencil, blending, blending_color) !=
           std::tie(other.program_ptr, other.rasterizer, other.depth, other.stencil, other.blending, other.blending_color);
}

RenderStateSettings::operator std::string() const
{
    META_FUNCTION_TASK();
    return fmt::format("  - Program '{}';\n{};\n{};\n{}\n{}\n  - Blending color: {}.",
                       program_ptr->GetName(),
                       static_cast<std::string>(rasterizer),
                       static_cast<std::string>(depth),
                       static_cast<std::string>(stencil),
                       static_cast<std::string>(blending),
                       static_cast<std::string>(blending_color));
}

} // namespace Methane::Graphics::Rhi
