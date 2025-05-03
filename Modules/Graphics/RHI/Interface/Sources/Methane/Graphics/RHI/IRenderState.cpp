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
#include <fmt/ranges.h>

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

RasterizerSettings::operator std::string() const
{
    META_FUNCTION_TASK();
    return fmt::format("  - Rasterizer: front={}, cull={}, fill={}, sample_count={}, alpha_to_coverage={}",
                       (is_front_counter_clockwise ? "CW" : "CCW"),
                       magic_enum::enum_name(cull_mode), magic_enum::enum_name(fill_mode),
                       sample_count, alpha_to_coverage_enabled);
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

BlendingSettings::operator std::string() const
{
    META_FUNCTION_TASK();
    return is_independent
           ? fmt::format("  - Blending is independent for render targets:\n{}.", fmt::join(render_targets, ";\n"))
           : fmt::format("  - Blending is common for all render targets:\n{}.", static_cast<std::string>(render_targets[0]));
}

DepthSettings::operator std::string() const
{
    META_FUNCTION_TASK();
    if (!enabled)
        return "  - Depth is disabled";

    return fmt::format("  - Depth is enabled: write_enabled={}, compare={}",
                       write_enabled, magic_enum::enum_name(compare));
}

FaceOperations::operator std::string() const
{
    META_FUNCTION_TASK();
    return fmt::format("face operations: stencil_failure={}, stencil_pass={}, depth_failure={}, depth_stencil_pass={}, compare={}",
                       magic_enum::enum_name(stencil_failure), magic_enum::enum_name(stencil_pass),
                       magic_enum::enum_name(depth_failure), magic_enum::enum_name(depth_stencil_pass),
                       magic_enum::enum_name(compare));
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

    using enum Group;
    if (compare_groups.HasAnyBit(Program) && left.program_ptr.get() != right.program_ptr.get())
        changed_state_groups.SetBitOn(Program);

    if (compare_groups.HasAnyBit(Rasterizer) && left.rasterizer != right.rasterizer)
        changed_state_groups.SetBitOn(Rasterizer);

    if (compare_groups.HasAnyBit(Blending) && left.blending != right.blending)
        changed_state_groups.SetBitOn(Blending);

    if (compare_groups.HasAnyBit(BlendingColor) && left.blending_color != right.blending_color)
        changed_state_groups.SetBitOn(BlendingColor);

    if (compare_groups.HasAnyBit(DepthStencil) && (left.depth != right.depth || left.stencil != right.stencil))
        changed_state_groups.SetBitOn(DepthStencil);

    return changed_state_groups;
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
