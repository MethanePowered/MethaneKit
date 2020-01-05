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

FILE: Methane/Graphics/RenderStateBase.cpp
Base implementation of the render state interface.

******************************************************************************/

#include "RenderStateBase.h"

#include <Methane/Data/Instrumentation.h>

#include <cassert>

namespace Methane::Graphics
{

RenderStateBase::RenderStateBase(ContextBase& context, const Settings& settings)
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

RenderState::Group::Mask RenderState::Settings::Compare(const Settings& left, const Settings& right, Group::Mask compare_groups)
{
    Group::Mask changed_state_groups = Group::ProgramBindingsOnly;
    if (compare_groups & Group::Program &&
        left.sp_program.get() != right.sp_program.get())
    {
        changed_state_groups |= Group::Program;
    }
    if (compare_groups & Group::DepthStencil && (
        left.depth   != right.depth ||
        left.stencil != right.stencil))
    {
        changed_state_groups |= Group::DepthStencil;
    }
    if (compare_groups & Group::Rasterizer && 
        left.rasterizer != right.rasterizer)
    {
        changed_state_groups |= Group::Rasterizer;
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

} // namespace Methane::Graphics
