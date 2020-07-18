/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/UserInterface/Item.cpp
Methane user interface item - base type of all user interface widgets and text.

******************************************************************************/

#include <Methane/UserInterface/Item.h>

#include <Methane/Instrumentation.h>

namespace Methane::UserInterface
{

Item::Item(Context& ui_context, const gfx::FrameRect& rect)
    : m_ui_context(ui_context)
    , m_rect(rect)
{
    META_FUNCTION_TASK();
}

bool Item::SetRect(const gfx::FrameRect& rect)
{
    META_FUNCTION_TASK();
    if (m_rect == rect)
        return false;

    m_rect = rect;
    Emit(&IItemCallback::RectChanged, *this);

    return true;
}

bool Item::SetOrigin(const gfx::FrameRect::Point& origin)
{
    META_FUNCTION_TASK();
    return SetRect({ origin, m_rect.size });
}

} // namespace Methane::UserInterface
