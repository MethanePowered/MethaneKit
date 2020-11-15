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
#include <Methane/Graphics/RenderContext.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::UserInterface
{

Context::Context(gfx::RenderContext& render_context) noexcept
    : m_render_context(render_context)
    , m_dots_to_pixels_factor(render_context.GetContentScalingFactor())
    , m_font_resolution_dpi(render_context.GetFontResolutionDpi())
{
    META_FUNCTION_TASK();
}

const FrameSize& Context::GetFrameSize() const noexcept
{
    return m_render_context.GetSettings().frame_size;
}

UnitSize Context::GetFrameSizeInUnits(Units units) const noexcept
{
    META_FUNCTION_TASK();
    switch(units)
    {
    case Units::Pixels: return GetFrameSizeInPixels();
    case Units::Dots:   return GetFrameSizeInDots();
    default:            return UnitSize();
    }
}

UnitPoint Context::ConvertToPixels(const FloatPoint& point) const noexcept
{
    META_FUNCTION_TASK();
    return UnitPoint(GetFrameSizeInPixels() * point);
}

UnitPoint Context::ConvertToDots(const FloatPoint& point) const noexcept
{
    META_FUNCTION_TASK();
    return UnitPoint(GetFrameSizeInDots() * point);
}

UnitSize Context::ConvertToPixels(const FloatSize& fsize) const noexcept
{
    META_FUNCTION_TASK();
    return UnitSize(GetFrameSizeInPixels() * fsize);
}

UnitSize Context::ConvertToDots(const FloatSize& fsize) const noexcept
{
    META_FUNCTION_TASK();
    return GetFrameSizeInDots() * fsize;
}

UnitRect Context::ConvertToPixels(const FloatRect& rect) const noexcept
{
    META_FUNCTION_TASK();
    const UnitSize frame_size_in_pixels = GetFrameSizeInPixels();
    const UnitSize origin_in_pixels = frame_size_in_pixels * rect.origin;
    return UnitRect(Units::Pixels, FramePoint(origin_in_pixels.width, origin_in_pixels.height), frame_size_in_pixels * rect.size);
}

UnitRect Context::ConvertToDots(const FloatRect& rect) const noexcept
{
    META_FUNCTION_TASK();
    const UnitSize frame_size_in_dots = GetFrameSizeInDots();
    const UnitSize origin_in_dots = frame_size_in_dots * rect.origin;
    return UnitRect(Units::Dots, FramePoint(origin_in_dots.width, origin_in_dots.height), frame_size_in_dots * rect.size);
}

UnitPoint Context::ConvertToPixels(const UnitPoint& point) const noexcept
{
    META_FUNCTION_TASK();
    return point.units == Units::Pixels ? point : UnitPoint(Units::Pixels, point * m_dots_to_pixels_factor);
}

UnitPoint Context::ConvertToDots(const UnitPoint& point) const noexcept
{
    META_FUNCTION_TASK();
    return point.units == Units::Dots ? point : UnitPoint(Units::Dots, point / m_dots_to_pixels_factor);
}

UnitPoint Context::ConvertToDots(const FramePoint& point_px) const noexcept
{
    META_FUNCTION_TASK();
    return UnitPoint(Units::Dots, point_px / m_dots_to_pixels_factor);
}

UnitSize Context::ConvertToPixels(const UnitSize& size) const noexcept
{
    META_FUNCTION_TASK();
    return size.units == Units::Pixels ? size : UnitSize(Units::Pixels, size * m_dots_to_pixels_factor);
}

UnitSize Context::ConvertToDots(const UnitSize& size) const noexcept
{
    META_FUNCTION_TASK();
    return size.units == Units::Dots ? size : UnitSize(Units::Dots, size / m_dots_to_pixels_factor);
}

UnitSize Context::ConvertToDots(const FrameSize& size_px) const noexcept
{
    META_FUNCTION_TASK();
    return UnitSize(Units::Dots, size_px / m_dots_to_pixels_factor);
}

UnitRect Context::ConvertToPixels(const UnitRect& rect) const noexcept
{
    META_FUNCTION_TASK();
    return rect.units == Units::Pixels ? rect : UnitRect(Units::Pixels, rect  * m_dots_to_pixels_factor);
}

UnitRect Context::ConvertToDots(const UnitRect& rect) const noexcept
{
    META_FUNCTION_TASK();
    return rect.units == Units::Dots ? rect : UnitRect(Units::Dots, rect / m_dots_to_pixels_factor);
}

UnitRect Context::ConvertToDots(const FrameRect& rect_px) const noexcept
{
    META_FUNCTION_TASK();
    return UnitRect(Units::Dots, rect_px / m_dots_to_pixels_factor);
}

UnitPoint Context::ConvertToUnits(const FramePoint& point_px, Units units) const noexcept
{
    META_FUNCTION_TASK();
    switch(units)
    {
    case Units::Pixels: return UnitPoint(units, point_px);
    case Units::Dots:   return ConvertToDots(point_px);
    default:            return UnitPoint();
    }
}

UnitSize Context::ConvertToUnits(const FrameSize& size_px, Units units) const noexcept
{
    META_FUNCTION_TASK();
    switch(units)
    {
    case Units::Pixels: return UnitSize(units, size_px);
    case Units::Dots:   return ConvertToDots(size_px);
    default:            return UnitSize();
    }
}

UnitRect Context::ConvertToUnits(const FrameRect& rect_px, Units units) const noexcept
{
    META_FUNCTION_TASK();
    switch(units)
    {
    case Units::Pixels: return UnitRect(units, rect_px);
    case Units::Dots:   return ConvertToDots(rect_px);
    default:            return UnitRect();
    }
}

} // namespace Methane::UserInterface
