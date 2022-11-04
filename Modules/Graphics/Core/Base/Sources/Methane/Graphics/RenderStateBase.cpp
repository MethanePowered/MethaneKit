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

#include <Methane/Graphics/RenderStateBase.h>

#include <Methane/Data/BitMaskHelpers.hpp>
#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>

#include <magic_enum.hpp>

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

RenderStateBase::RenderStateBase(const RenderContextBase& context, const Settings& settings)
    : m_context(context)
    , m_settings(settings)
{
    META_FUNCTION_TASK();
}

void RenderStateBase::Reset(const Settings& settings)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL_DESCR(settings.program_ptr, "program is not initialized in render state settings");
    META_CHECK_ARG_NOT_NULL_DESCR(settings.render_pattern_ptr, "render pass pattern is not initialized in render state settings");

    m_settings = settings;
}

IProgram& RenderStateBase::GetProgram()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_settings.program_ptr);
    return *m_settings.program_ptr;
}

} // namespace Methane::Graphics
