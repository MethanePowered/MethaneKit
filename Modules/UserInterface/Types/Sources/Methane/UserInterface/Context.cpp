/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/UserInterface/Context.cpp
Methane user interface context used by all widgets for rendering.

******************************************************************************/

#include <Methane/UserInterface/Context.h>
#include <Methane/Platform/IApp.h>
#include <Methane/Instrumentation.h>

namespace Methane::UserInterface
{

Context::Context(const pal::IApp& app, const rhi::CommandQueue& render_cmd_queue, const rhi::RenderPattern& render_pattern)
    : m_render_context(render_pattern.GetRenderContext())
    , m_render_cmd_queue(render_cmd_queue)
    , m_render_pattern(render_pattern)
    , m_dots_to_pixels_factor(app.GetContentScalingFactor())
    , m_font_resolution_dpi(app.GetFontResolutionDpi())
{
    META_FUNCTION_TASK();
    META_CHECK_EQUAL(render_cmd_queue.GetCommandListType(), rhi::CommandListType::Render);
}

UnitSize Context::GetFrameSizeInUnits(Units units) const
{
    META_FUNCTION_TASK();
    switch(units)
    {
    using enum Units;
    case Pixels: return GetFrameSizeIn<Pixels>();
    case Dots:   return GetFrameSizeIn<Dots>();
    default:     return UnitSize();
    }
}

} // namespace Methane::UserInterface
