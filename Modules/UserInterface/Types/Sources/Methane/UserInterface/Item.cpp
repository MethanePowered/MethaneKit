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

FILE: Methane/UserInterface/Item.cpp
Methane user interface item - base type of all user interface widgets and text.

******************************************************************************/

#include <Methane/UserInterface/Item.h>
#include <Methane/UserInterface/Context.h>

#include <Methane/Instrumentation.h>

namespace Methane::UserInterface
{

Item::Item(Context& ui_context, const UnitRect& ui_rect)
    : m_ui_context(ui_context)
    , m_abs_rect_px(m_ui_context.ConvertToPixels(ui_rect))
{
    META_FUNCTION_TASK();
}

UnitPoint Item::GetRelOriginInDots() const noexcept
{
    META_FUNCTION_TASK();
    assert(m_rel_origin_px.units == Units::Pixels);
    return m_ui_context.ConvertToDots(m_rel_origin_px);
}

UnitPoint Item::GetRelOriginInUnits(Units units) const noexcept
{
    META_FUNCTION_TASK();
    assert(m_rel_origin_px.units == Units::Pixels);
    return m_ui_context.ConvertToUnits(m_rel_origin_px, units);
}

UnitRect Item::GetRectInDots() const noexcept
{
    META_FUNCTION_TASK();
    assert(m_abs_rect_px.units == Units::Pixels);
    return m_ui_context.ConvertToDots(m_abs_rect_px);
}

UnitRect Item::GetRectInUnits(Units units) const noexcept
{
    META_FUNCTION_TASK();
    return m_ui_context.ConvertToUnits(m_abs_rect_px, units);
}

bool Item::SetRect(const UnitRect& rect)
{
    META_FUNCTION_TASK();
    if (m_abs_rect_px == rect)
        return false;

    const UnitRect rect_px = m_ui_context.ConvertToPixels(rect);
    if (m_abs_rect_px == rect_px)
        return false;

    m_abs_rect_px = rect_px;
    Emit(&IItemCallback::RectChanged, *this);

    return true;
}

void Item::SetRelOrigin(const UnitPoint& rel_origin)
{
    META_FUNCTION_TASK();
    m_rel_origin_px = m_ui_context.ConvertToPixels(rel_origin);
}

bool Item::SetOrigin(const UnitPoint& origin)
{
    META_FUNCTION_TASK();
    assert(m_abs_rect_px.units == Units::Pixels);
    return SetRect(UnitRect(m_abs_rect_px.units, m_ui_context.ConvertToPixels(origin), m_abs_rect_px.size));
}

bool Item::SetSize(const UnitSize& size)
{
    META_FUNCTION_TASK();
    assert(m_abs_rect_px.units == Units::Pixels);
    return SetRect(UnitRect(m_abs_rect_px.units, m_abs_rect_px.origin, m_ui_context.ConvertToPixels(size)));
}

} // namespace Methane::UserInterface
