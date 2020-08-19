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

#define COMPARE_STATE_GROUPS_WITH_MEMCMP

#ifdef COMPARE_STATE_GROUPS_WITH_MEMCMP
#include <cstring>
#endif

namespace Methane::Graphics
{

inline void Validate(const Viewports& viewports)
{
    if (viewports.empty())
    {
        throw std::invalid_argument("Can not set empty viewports to state.");
    }
}

inline void Validate(const ScissorRects& scissor_rects)
{
    if (scissor_rects.empty())
    {
        throw std::invalid_argument("Can not set empty scissor rectangles to state.");
    }
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

bool RenderState::Rasterizer::operator==(const Rasterizer& other) const noexcept
{
    META_FUNCTION_TASK();
#ifdef COMPARE_STATE_GROUPS_WITH_MEMCMP
    return std::memcmp(this, &other, sizeof(RenderState::Rasterizer)) == 0;
#else
    return std::tie(is_front_counter_clockwise, cull_mode, fill_mode, sample_count, alpha_to_coverage_enabled) ==
           std::tie(other.is_front_counter_clockwise, other.cull_mode, other.fill_mode, other.sample_count, other.alpha_to_coverage_enabled);
#endif
}

bool RenderState::Rasterizer::operator!=(const Rasterizer& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

bool RenderState::Blending::RenderTarget::operator==(const RenderTarget& other) const noexcept
{
    META_FUNCTION_TASK();
#ifdef COMPARE_STATE_GROUPS_WITH_MEMCMP
    return std::memcmp(this, &other, sizeof(RenderState::Blending::RenderTarget)) == 0;
#else
    return std::tie(blend_enabled, write_mask, rgb_blend_op, alpha_blend_op, 
                    source_rgb_blend_factor, source_alpha_blend_factor, dest_rgb_blend_factor, dest_alpha_blend_factor) ==
           std::tie(other.blend_enabled, other.write_mask, other.rgb_blend_op, other.alpha_blend_op,
                    other.source_rgb_blend_factor, other.source_alpha_blend_factor, other.dest_rgb_blend_factor, other.dest_alpha_blend_factor);
#endif
}

bool RenderState::Blending::RenderTarget::operator!=(const RenderTarget& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

bool RenderState::Blending::operator==(const Blending& other) const noexcept
{
    META_FUNCTION_TASK();
#ifdef COMPARE_STATE_GROUPS_WITH_MEMCMP
    return std::memcmp(this, &other, sizeof(RenderState::Blending)) == 0;
#else
    return std::tie(is_independent, render_targets) ==
           std::tie(other.is_independent, other.render_targets);
#endif
}

bool RenderState::Blending::operator!=(const Blending& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

bool RenderState::Depth::operator==(const Depth& other) const noexcept
{
    META_FUNCTION_TASK();
#ifdef COMPARE_STATE_GROUPS_WITH_MEMCMP
    return std::memcmp(this, &other, sizeof(RenderState::Depth)) == 0;
#else
    return std::tie(enabled, write_enabled, compare) ==
           std::tie(other.enabled, other.write_enabled, other.compare);
#endif
}
bool RenderState::Depth::operator!=(const Depth& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

bool RenderState::Stencil::FaceOperations::operator==(const FaceOperations& other) const noexcept
{
    META_FUNCTION_TASK();
#ifdef COMPARE_STATE_GROUPS_WITH_MEMCMP
    return std::memcmp(this, &other, sizeof(RenderState::Stencil::FaceOperations)) == 0;
#else
    return std::tie(stencil_failure, stencil_pass, depth_failure, depth_stencil_pass, compare) ==
           std::tie(other.stencil_failure, other.stencil_pass, other.depth_failure, other.depth_stencil_pass, other.compare);
#endif
}

bool RenderState::Stencil::FaceOperations::operator!=(const FaceOperations& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

bool RenderState::Stencil::operator==(const Stencil& other) const noexcept
{
    META_FUNCTION_TASK();
#ifdef COMPARE_STATE_GROUPS_WITH_MEMCMP
    return std::memcmp(this, &other, sizeof(RenderState::Stencil)) == 0;
#else
    return std::tie(enabled, read_mask, write_mask, front_face, back_face) ==
           std::tie(other.enabled, other.read_mask, other.write_mask, other.front_face, other.back_face);
#endif
}

bool RenderState::Stencil::operator!=(const Stencil& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

RenderState::Group::Mask RenderState::Settings::Compare(const Settings& left, const Settings& right, Group::Mask compare_groups) noexcept
{
    META_FUNCTION_TASK();
    
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
    
    return changed_state_groups;
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
    assert(!!m_settings.sp_program);
    return *m_settings.sp_program;
}

} // namespace Methane::Graphics
