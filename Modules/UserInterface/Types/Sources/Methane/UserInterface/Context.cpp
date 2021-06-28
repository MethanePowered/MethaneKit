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
#include <Methane/Instrumentation.h>

namespace Methane::UserInterface
{

Context::Context(gfx::RenderPattern& render_pattern) noexcept
    : m_render_pattern_ptr(std::dynamic_pointer_cast<gfx::RenderPattern>(render_pattern.GetPtr()))
    , m_dots_to_pixels_factor(render_pattern.GetRenderContext().GetContentScalingFactor())
    , m_font_resolution_dpi(render_pattern.GetRenderContext().GetFontResolutionDpi())
{
    META_FUNCTION_TASK();
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
