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

FILE: Methane/UserInterface/TextItem.h
UI text item.

******************************************************************************/

#pragma once

#include <Methane/UserInterface/Item.h>
#include <Methane/UserInterface/Text.h>
#include <Methane/Data/Receiver.hpp>

namespace Methane::UserInterface
{

class TextItem final
    : public Text
    , public Item
    , private Data::Receiver<ITextCallback>
{
public:
    TextItem(Context& ui_context, const rhi::RenderPattern& render_pattern, const Font& font, const SettingsUtf8& settings);
    TextItem(Context& ui_context, const Font& font, const SettingsUtf8& settings);
    TextItem(Context& ui_context, const rhi::RenderPattern& render_pattern, const Font& font, const SettingsUtf32& settings);
    TextItem(Context& ui_context, const Font& font, const SettingsUtf32& settings);

    // Item overrides
    bool SetRect(const UnitRect& ui_rect) override;

private:
    // ITextCallback overrides
    void OnTextFrameRectChanged(const UnitRect& frame_rect) override;
};

} // namespace Methane::UserInterface
