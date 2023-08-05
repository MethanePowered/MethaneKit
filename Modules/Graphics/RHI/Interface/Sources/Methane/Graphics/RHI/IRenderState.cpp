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
#include <Methane/Graphics/RHI/IRenderContext.h>
#include <Methane/Graphics/RHI/IProgram.h>

#include <Methane/Data/EnumMaskUtil.hpp>
#include <Methane/Instrumentation.h>

#include <fmt/format.h>

template<>
struct fmt::formatter<Methane::Graphics::Rhi::RenderTargetSettings>
{
    template<typename FormatContext>
    [[nodiscard]] auto format(const Methane::Graphics::Rhi::RenderTargetSettings& rt, FormatContext& ctx) const
    { return format_to(ctx.out(), "{}", static_cast<std::string>(rt)); }

    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const
    { return ctx.end(); }
};

namespace Methane::Graphics::Rhi
{

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

bool RenderTargetSettings::operator==(const RenderTargetSettings& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(blend_enabled, color_write, rgb_blend_op, alpha_blend_op,
                    source_rgb_blend_factor, source_alpha_blend_factor,
                    dest_rgb_blend_factor, dest_alpha_blend_factor) ==
           std::tie(other.blend_enabled, other.color_write, other.rgb_blend_op, other.alpha_blend_op,
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
                       Data::GetEnumMaskName(color_write),
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

RenderStateGroupMask RenderStateSettings::Compare(const RenderStateSettings& left, const RenderStateSettings& right, GroupMask compare_groups) noexcept
{
    META_FUNCTION_TASK();
    GroupMask changed_state_groups;

    if (compare_groups.HasAnyBit(Group::Program) && left.program_ptr.get() != right.program_ptr.get())
        changed_state_groups.SetBitOn(Group::Program);

    if (compare_groups.HasAnyBit(Group::Rasterizer) && left.rasterizer != right.rasterizer)
        changed_state_groups.SetBitOn(Group::Rasterizer);

    if (compare_groups.HasAnyBit(Group::Blending) && left.blending != right.blending)
        changed_state_groups.SetBitOn(Group::Blending);

    if (compare_groups.HasAnyBit(Group::BlendingColor) && left.blending_color != right.blending_color)
        changed_state_groups.SetBitOn(Group::BlendingColor);

    if (compare_groups.HasAnyBit(Group::DepthStencil) && (left.depth != right.depth || left.stencil != right.stencil))
        changed_state_groups.SetBitOn(Group::DepthStencil);

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

Ptr<IRenderState> IRenderState::Create(const IRenderContext& context, const Settings& state_settings)
{
    META_FUNCTION_TASK();
    return context.CreateRenderState(state_settings);
}

} // namespace Methane::Graphics::Rhi
