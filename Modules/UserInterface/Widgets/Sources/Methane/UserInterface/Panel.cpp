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

FILE: Methane/UserInterface/Panel.cpp
Panel widget with opaque background containing other widgets.

******************************************************************************/

#include <Methane/UserInterface/Panel.h>
#include <Methane/UserInterface/Context.h>
#include <Methane/Instrumentation.h>

#include <cmath>

namespace Methane::UserInterface
{

Panel::Panel(Context& ui_context, const gfx::FrameRect& rect, Settings settings)
    : Container(ui_context, rect)
    , ScreenQuad(ui_context.GetRenderContext(),
        ScreenQuad::Settings
        {
            settings.name,
            rect,
            true, // alpha_blending_enabled
            settings.background_color,
            TextureMode::Disabled
        }
    )
    , m_settings(std::move(settings))
{
    META_FUNCTION_TASK();
}

// Item overrides
bool Panel::SetRect(const gfx::FrameRect& rect)
{
    META_FUNCTION_TASK();
    if (!Container::SetRect(rect))
        return false;

    gfx::ScreenQuad::SetScreenRect(rect);
    return true;
}

} // namespace Methane::UserInterface
