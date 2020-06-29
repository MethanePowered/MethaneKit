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

FILE: Methane/UserInterface/HeadsUpDisplay.h
HeadsUpDisplay rendering primitive.

******************************************************************************/

#pragma once

#include <Methane/UserInterface/Text.h>
#include <Methane/Graphics/Color.hpp>
#include <Methane/Timer.hpp>
#include <Methane/Memory.hpp>

namespace Methane::Graphics
{
struct RenderContext;
struct RenderCommandList;
}

namespace Methane::UserInterface
{

namespace gfx = Methane::Graphics;

class Font;

class HeadsUpDisplay
{
public:
    struct Settings
    {
        gfx::Point2i position            = { 20, 20 };
        gfx::Color4f text_color          = gfx::Color4f(1.f, 1.f, 1.f, 1.f);
        double  update_interval_sec = 0.33;
    };

    explicit HeadsUpDisplay(gfx::RenderContext& context);
    HeadsUpDisplay(gfx::RenderContext& context, Settings settings);

    const Settings& GetSettings() const { return m_settings; }

    void SetPosition(const gfx::Point2i& position);
    void SetTextColor(const gfx::Color4f& text_color);
    void SetUpdateInterval(double update_interval_sec);

    void Update();
    void Draw(gfx::RenderCommandList& cmd_list);

private:
    Settings            m_settings;
    gfx::RenderContext& m_context;
    const Ptr<Font>     m_sp_major_font;
    Text                m_fps_text;
    Timer               m_update_timer;
};

} // namespace Methane::UserInterface
