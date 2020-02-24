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

FILE: Methane/Graphics/RenderStateBase.cpp
Base implementation of the render state interface.

******************************************************************************/

#include "RenderStateBase.h"

#include <Methane/Instrumentation.h>

#include <cassert>

namespace Methane::Graphics
{

bool RenderState::Rasterizer::operator==(const Rasterizer& other) const noexcept
{
    ITT_FUNCTION_TASK();
    return std::tie(is_front_counter_clockwise, cull_mode, fill_mode, sample_count, alpha_to_coverage_enabled) ==
           std::tie(other.is_front_counter_clockwise, other.cull_mode, other.fill_mode, other.sample_count, other.alpha_to_coverage_enabled);
}

bool RenderState::Rasterizer::operator!=(const Rasterizer& other) const noexcept
{
    ITT_FUNCTION_TASK();
    return !operator==(other);
}

bool RenderState::Blending::RenderTarget::operator==(const RenderTarget& other) const noexcept
{
    ITT_FUNCTION_TASK();
    return std::tie(blend_enabled, write_mask, rgb_blend_op, alpha_blend_op, 
                    source_rgb_blend_factor, source_alpha_blend_factor, dest_rgb_blend_factor, dest_alpha_blend_factor) ==
           std::tie(other.blend_enabled, other.write_mask, other.rgb_blend_op, other.alpha_blend_op,
                    other.source_rgb_blend_factor, other.source_alpha_blend_factor, other.dest_rgb_blend_factor, other.dest_alpha_blend_factor);
}

bool RenderState::Blending::RenderTarget::operator!=(const RenderTarget& other) const noexcept
{
    ITT_FUNCTION_TASK();
    return !operator==(other);
}

bool RenderState::Blending::operator==(const Blending& other) const noexcept
{
    ITT_FUNCTION_TASK();
    return std::tie(is_independent, render_targets) ==
           std::tie(other.is_independent, other.render_targets);
}

bool RenderState::Blending::operator!=(const Blending& other) const noexcept
{
    ITT_FUNCTION_TASK();
    return !operator==(other);
}

bool RenderState::Depth::operator==(const Depth& other) const noexcept
{
    ITT_FUNCTION_TASK();
    return std::tie(enabled, write_enabled, compare) ==
           std::tie(other.enabled, other.write_enabled, other.compare);
}
bool RenderState::Depth::operator!=(const Depth& other) const noexcept
{
    ITT_FUNCTION_TASK();
    return !operator==(other);
}

bool RenderState::Stencil::FaceOperations::operator==(const FaceOperations& other) const noexcept
{
    ITT_FUNCTION_TASK();
    return std::tie(stencil_failure, stencil_pass, depth_failure, depth_stencil_pass, compare) ==
           std::tie(other.stencil_failure, other.stencil_pass, other.depth_failure, other.depth_stencil_pass, other.compare);
}

bool RenderState::Stencil::FaceOperations::operator!=(const FaceOperations& other) const noexcept
{
    ITT_FUNCTION_TASK();
    return !operator==(other);
}

bool RenderState::Stencil::operator==(const Stencil& other) const noexcept
{
    ITT_FUNCTION_TASK();
    return std::tie(enabled, read_mask, write_mask, front_face, back_face) ==
           std::tie(other.enabled, other.read_mask, other.write_mask, other.front_face, other.back_face);
}
bool RenderState::Stencil::operator!=(const Stencil& other) const noexcept
{
    ITT_FUNCTION_TASK();
    return !operator==(other);
}

RenderState::Group::Mask RenderState::Settings::Compare(const Settings& left, const Settings& right, Group::Mask compare_groups) noexcept
{
    ITT_FUNCTION_TASK();
    
    Group::Mask changed_state_groups = Group::None;
    
    if (compare_groups & Group::Program &&
        left.sp_program.get() != right.sp_program.get())
    {
        changed_state_groups |= Group::Program;
    }
    if (compare_groups & Group::Rasterizer &&
        left.rasterizer != right.rasterizer)
    {
        changed_state_groups |= Group::Rasterizer;
    }
    if (compare_groups & Group::Blending &&
        left.blending != right.blending)
    {
        changed_state_groups |= Group::Blending;
    }
    if (compare_groups & Group::BlendingColor &&
        left.blending_color != right.blending_color)
    {
        changed_state_groups |= Group::BlendingColor;
    }
    if (compare_groups & Group::DepthStencil && (
        left.depth   != right.depth ||
        left.stencil != right.stencil))
    {
        changed_state_groups |= Group::DepthStencil;
    }
    if (compare_groups & Group::Viewports &&
        left.viewports != right.viewports)
    {
        changed_state_groups |= Group::Viewports;
    }
    if (compare_groups & Group::ScissorRects &&
        left.scissor_rects != right.scissor_rects)
    {
        changed_state_groups |= Group::ScissorRects;
    }
    
    return changed_state_groups;
}
    
RenderStateBase::RenderStateBase(RenderContextBase& context, const Settings& settings)
    : m_context(context)
    , m_settings(settings)
{
    ITT_FUNCTION_TASK();
}

void RenderStateBase::Reset(const Settings& settings)
{
    ITT_FUNCTION_TASK();
    m_settings = settings;
}

void RenderStateBase::SetViewports(const Viewports& viewports)
{
    ITT_FUNCTION_TASK();
    if (viewports.empty())
    {
        throw std::invalid_argument("Can not set empty viewports to state.");
    }
    m_settings.viewports = viewports;
}

void RenderStateBase::SetScissorRects(const ScissorRects& scissor_rects)
{
    ITT_FUNCTION_TASK();
    if (scissor_rects.empty())
    {
        throw std::invalid_argument("Can not set empty scissor rectangles to state.");
    }
    m_settings.scissor_rects = scissor_rects;
}

Program& RenderStateBase::GetProgram()
{
    ITT_FUNCTION_TASK();
    assert(!!m_settings.sp_program);
    return *m_settings.sp_program;
}

} // namespace Methane::Graphics
