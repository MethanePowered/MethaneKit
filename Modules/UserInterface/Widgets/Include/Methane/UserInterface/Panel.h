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

FILE: Methane/UserInterface/Panel.h
Panel widget with opaque background containing other widgets.

******************************************************************************/

#pragma once

#include <Methane/UserInterface/Container.h>

#include <Methane/Graphics/ScreenQuad.h>

namespace Methane::Graphics
{
class ImageLoader;
}

namespace Methane::UserInterface
{

class Panel
    : public Container
    , public gfx::ScreenQuad
{
public:
    struct Settings
    {
        std::string  name;
        gfx::Color4f background_color { 0.F, 0.F, 0.F, 0.66F };
    };

    Panel(Context& ui_context, const UnitRect& rect, Settings settings);

    const Settings& GetSettings() const noexcept { return m_settings; }

    // Item overrides
    bool SetRect(const UnitRect& ui_rect) override;

protected:
    using gfx::ScreenQuad::SetScreenRect;

private:
    Settings m_settings;
};

} // namespace Methane::UserInterface
