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

FILE: Methane/UserInterface/IContext.cpp
Methane user interface context used by all widgets for rendering.

******************************************************************************/

#include <Methane/UserInterface/Context.h>
#include <Methane/Platform/IApp.h>
#include <Methane/Graphics/IRenderContext.h>
#include <Methane/Graphics/ICommandQueue.h>
#include <Methane/Graphics/IRenderPass.h>
#include <Methane/Instrumentation.h>

namespace Methane::UserInterface
{

Context::Context(const pal::IApp& app, gfx::ICommandQueue& render_cmd_queue, gfx::IRenderPattern& render_pattern)
    : m_render_context(render_pattern.GetRenderContext())
    , m_render_cmd_queue_ptr(std::dynamic_pointer_cast<gfx::ICommandQueue>(render_cmd_queue.GetPtr()))
    , m_render_pattern_ptr(std::dynamic_pointer_cast<gfx::IRenderPattern>(render_pattern.GetPtr()))
    , m_dots_to_pixels_factor(app.GetContentScalingFactor())
    , m_font_resolution_dpi(app.GetFontResolutionDpi())
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL(render_cmd_queue.GetCommandListType(), gfx::CommandList::Type::Render);
}

UnitSize Context::GetFrameSizeInUnits(Units units) const noexcept
{
    META_FUNCTION_TASK();
    switch(units)
    {
    case Units::Pixels: return GetFrameSizeIn<Units::Pixels>();
    case Units::Dots:   return GetFrameSizeIn<Units::Dots>();
    default:            return UnitSize();
    }
}

} // namespace Methane::UserInterface
