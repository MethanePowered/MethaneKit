/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/UserInterface/Badge.cpp
Badge widget displaying texture in specific corner of the screen.

******************************************************************************/

#include <Methane/UserInterface/TextItem.h>
#include <Methane/UserInterface/Context.h>

namespace Methane::UserInterface
{

TextItem::TextItem(Context& ui_context, const rhi::RenderPattern& render_pattern, const Font& font, const SettingsUtf8& settings)
    : Text(ui_context, render_pattern, font, settings)
    , Item(ui_context, Text::GetFrameRect())
{
    Item::SetRelOrigin(settings.rect.GetUnitOrigin());
    Item::SetRect(Text::GetFrameRect());
    Text::Connect(*this);
}

TextItem::TextItem(Context& ui_context, const Font& font, const SettingsUtf8& settings)
    : TextItem(ui_context, ui_context.GetRenderPattern(), font, settings)
{
}

TextItem::TextItem(Context& ui_context, const rhi::RenderPattern& render_pattern, const Font& font, const SettingsUtf32& settings)
    : Text(ui_context, render_pattern, font, settings)
    , Item(ui_context, Text::GetFrameRect())
{
    Item::SetRelOrigin(settings.rect.GetUnitOrigin());
    Item::SetRect(Text::GetFrameRect());
    Text::Connect(*this);
}

TextItem::TextItem(Context& ui_context, const Font& font, const SettingsUtf32& settings)
    : TextItem(ui_context, ui_context.GetRenderPattern(), font, settings)
{
}

bool TextItem::SetRect(const UnitRect& ui_rect)
{
    META_FUNCTION_TASK();
    Text::SetFrameRect(ui_rect);
    return Item::SetRect(Text::GetFrameRect());
}

void TextItem::OnTextFrameRectChanged(const UnitRect& frame_rect)
{
    META_FUNCTION_TASK();
    Item::SetRect(frame_rect);
}

} // namespace Methane::UserInterface
