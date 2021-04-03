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

FILE: Methane/Graphics/RenderStateBase.cpp
Base implementation of the render state interface.

******************************************************************************/

#include "RenderStateBase.h"

#include <Methane/Graphics/TypeFormatters.hpp>
#include <Methane/Data/BitMaskHelpers.hpp>
#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>

#include <magic_enum.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>

template<>
struct fmt::formatter<Methane::Graphics::RenderState::Blending::RenderTarget>
{
    template<typename FormatContext>
    [[nodiscard]] auto format(const Methane::Graphics::RenderState::Blending::RenderTarget& rt, FormatContext& ctx) { return format_to(ctx.out(), "{}", static_cast<std::string>(rt)); }
    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }
};

namespace Methane::Graphics
{

inline void Validate(const Viewports& viewports)
{
    META_CHECK_ARG_NOT_EMPTY_DESCR(viewports, "can not set empty viewports to state");
}

inline void Validate(const ScissorRects& scissor_rects)
{
    META_CHECK_ARG_NOT_EMPTY_DESCR(scissor_rects, "can not set empty scissor rectangles to state");
}

ViewStateBase::ViewStateBase(const Settings& settings)
    : m_settings(settings)
{
    META_FUNCTION_TASK();
    Validate(settings.viewports);
    Validate(settings.scissor_rects);
}

bool ViewStateBase::Reset(const Settings& settings)
{
    META_FUNCTION_TASK();
    if (m_settings == settings)
        return false;

    Validate(settings.viewports);
    Validate(settings.scissor_rects);

    m_settings = settings;
    return true;
}

bool ViewStateBase::SetViewports(const Viewports& viewports)
{
    META_FUNCTION_TASK();
    if (m_settings.viewports == viewports)
        return false;

    Validate(viewports);
    m_settings.viewports = viewports;
    return true;
}

bool ViewStateBase::SetScissorRects(const ScissorRects& scissor_rects)
{
    META_FUNCTION_TASK();
    if (m_settings.scissor_rects == scissor_rects)
        return false;

    Validate(scissor_rects);
    m_settings.scissor_rects = scissor_rects;
    return true;
}

ViewState::Settings::operator std::string() const
{
    META_FUNCTION_TASK();
    return fmt::format("  - Viewports: {};\n  - Scissor Rects: {}.", fmt::join(viewports, ", "), fmt::join(scissor_rects, ", "));
}

bool RenderState::Rasterizer::operator==(const Rasterizer& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(is_front_counter_clockwise, cull_mode, fill_mode, sample_count, alpha_to_coverage_enabled) ==
           std::tie(other.is_front_counter_clockwise, other.cull_mode, other.fill_mode, other.sample_count, other.alpha_to_coverage_enabled);
}

bool RenderState::Rasterizer::operator!=(const Rasterizer& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

RenderState::Rasterizer::operator std::string() const
{
    META_FUNCTION_TASK();
    return fmt::format("  - Rasterizer: front={}, cull={}, fill={}, sample_count={}, alpha_to_coverage={}",
                       (is_front_counter_clockwise ? "CW" : "CCW"),
                       magic_enum::enum_name(cull_mode), magic_enum::enum_name(fill_mode),
                       sample_count, alpha_to_coverage_enabled);
}

bool RenderState::Blending::RenderTarget::operator==(const RenderTarget& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(blend_enabled, write_mask, rgb_blend_op, alpha_blend_op, 
                    source_rgb_blend_factor, source_alpha_blend_factor, dest_rgb_blend_factor, dest_alpha_blend_factor) ==
           std::tie(other.blend_enabled, other.write_mask, other.rgb_blend_op, other.alpha_blend_op,
                    other.source_rgb_blend_factor, other.source_alpha_blend_factor, other.dest_rgb_blend_factor, other.dest_alpha_blend_factor);
}

bool RenderState::Blending::RenderTarget::operator!=(const RenderTarget& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

RenderState::Blending::RenderTarget::operator std::string() const
{
    META_FUNCTION_TASK();
    if (!blend_enabled)
        return "    - Render Target blending is disabled";

    return fmt::format("    - Render Target blending: write_mask={}, rgb_blend_op={}, alpha_blend_op={}, "
                       "source_rgb_blend_factor={}, source_alpha_blend_factor={}, dest_rgb_blend_factor={}, dest_alpha_blend_factor={}",
                       Data::GetBitMaskFlagNames(write_mask),
                       magic_enum::enum_name(rgb_blend_op), magic_enum::enum_name(alpha_blend_op),
                       magic_enum::enum_name(source_rgb_blend_factor), magic_enum::enum_name(source_alpha_blend_factor),
                       magic_enum::enum_name(dest_rgb_blend_factor), magic_enum::enum_name(dest_alpha_blend_factor));
}

bool RenderState::Blending::operator==(const Blending& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(is_independent, render_targets) ==
           std::tie(other.is_independent, other.render_targets);
}

bool RenderState::Blending::operator!=(const Blending& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

RenderState::Blending::operator std::string() const
{
    META_FUNCTION_TASK();
    return is_independent
         ? fmt::format("  - Blending is independent for render targets:\n{}.", fmt::join(render_targets, ";\n"))
         : fmt::format("  - Blending is common for all render targets:\n{}.", static_cast<std::string>(render_targets[0]));
}

bool RenderState::Depth::operator==(const Depth& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(enabled, write_enabled, compare) ==
           std::tie(other.enabled, other.write_enabled, other.compare);
}
bool RenderState::Depth::operator!=(const Depth& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

RenderState::Depth::operator std::string() const
{
    META_FUNCTION_TASK();
    if (!enabled)
        return "  - Depth is disabled";

    return fmt::format("  - Depth is enabled: write_enabled={}, compare={}",
                       write_enabled, magic_enum::enum_name(compare));
}

bool RenderState::Stencil::FaceOperations::operator==(const FaceOperations& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(stencil_failure, stencil_pass, depth_failure, depth_stencil_pass, compare) ==
           std::tie(other.stencil_failure, other.stencil_pass, other.depth_failure, other.depth_stencil_pass, other.compare);
}

bool RenderState::Stencil::FaceOperations::operator!=(const FaceOperations& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

RenderState::Stencil::FaceOperations::operator std::string() const
{
    META_FUNCTION_TASK();
    return fmt::format("face operations: stencil_failure={}, stencil_pass={}, depth_failure={}, depth_stencil_pass={}, compare={}",
                       magic_enum::enum_name(stencil_failure), magic_enum::enum_name(stencil_pass),
                       magic_enum::enum_name(depth_failure), magic_enum::enum_name(depth_stencil_pass),
                       magic_enum::enum_name(compare));
}

bool RenderState::Stencil::operator==(const Stencil& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(enabled, read_mask, write_mask, front_face, back_face) ==
           std::tie(other.enabled, other.read_mask, other.write_mask, other.front_face, other.back_face);
}

bool RenderState::Stencil::operator!=(const Stencil& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

RenderState::Stencil::operator std::string() const
{
    META_FUNCTION_TASK();
    if (!enabled)
        return "  - Stencil is disabled";

    return fmt::format("  - Stencil is enabled: read_mask={:x}, write_mask={:x}, face operations:\n    - Front {};\n    - Back {}.",
                       read_mask, write_mask, static_cast<std::string>(front_face), static_cast<std::string>(back_face));
}

RenderState::Groups RenderState::Settings::Compare(const Settings& left, const Settings& right, Groups compare_groups) noexcept
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;
    
    Groups changed_state_groups = Groups::None;
    
    if (magic_enum::flags::enum_contains(compare_groups & Groups::Program) &&
        left.program_ptr.get() != right.program_ptr.get())
    {
        changed_state_groups |= Groups::Program;
    }
    if (magic_enum::flags::enum_contains(compare_groups & Groups::Rasterizer) &&
        left.rasterizer != right.rasterizer)
    {
        changed_state_groups |= Groups::Rasterizer;
    }
    if (magic_enum::flags::enum_contains(compare_groups & Groups::Blending) &&
        left.blending != right.blending)
    {
        changed_state_groups |= Groups::Blending;
    }
    if (magic_enum::flags::enum_contains(compare_groups & Groups::BlendingColor) &&
        left.blending_color != right.blending_color)
    {
        changed_state_groups |= Groups::BlendingColor;
    }
    if (magic_enum::flags::enum_contains(compare_groups & Groups::DepthStencil) &&
        (left.depth != right.depth || left.stencil != right.stencil))
    {
        changed_state_groups |= Groups::DepthStencil;
    }
    
    return changed_state_groups;
}

bool RenderState::Settings::operator==(const Settings& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(program_ptr, rasterizer, depth, stencil, blending, blending_color) ==
           std::tie(other.program_ptr, other.rasterizer, other.depth, other.stencil, other.blending, other.blending_color);
}

bool RenderState::Settings::operator!=(const Settings& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(program_ptr, rasterizer, depth, stencil, blending, blending_color) !=
           std::tie(other.program_ptr, other.rasterizer, other.depth, other.stencil, other.blending, other.blending_color);
}

RenderState::Settings::operator std::string() const
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

RenderStateBase::RenderStateBase(RenderContextBase& context, const Settings& settings)
    : m_context(context)
    , m_settings(settings)
{
    META_FUNCTION_TASK();
}

void RenderStateBase::Reset(const Settings& settings)
{
    META_FUNCTION_TASK();
    m_settings = settings;
}

Program& RenderStateBase::GetProgram()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_settings.program_ptr);
    return *m_settings.program_ptr;
}

} // namespace Methane::Graphics
