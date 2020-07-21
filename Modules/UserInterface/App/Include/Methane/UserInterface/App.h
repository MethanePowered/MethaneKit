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

FILE: Methane/UserInterface/App.h
Interface of the base user interface application template class defined in App.hpp

******************************************************************************/

#pragma once

#include <Methane/Graphics/App.h>
#include <Methane/UserInterface/Types.hpp>

namespace Methane::UserInterface
{

struct IApp : Graphics::IApp
{
    enum HeadsUpDisplayMode : uint32_t
    {
        Hidden = 0u,
        WindowTitle,
        UserInterface,

        Count
    };

    struct Settings
    {
        HeadsUpDisplayMode  heads_up_display_mode = HeadsUpDisplayMode::WindowTitle;
        bool                show_logo_badge       = true;
        Color4f             logo_badge_color      { 1.f, 1.f, 1.f, 0.15f };
        Color4f             text_color            { 1.f, 1.f, 1.f, 1.f };
        UnitPoint           text_margins          { 20, 20, Units::Dots };
    };

    virtual const IApp::Settings& GetUserInterfaceAppSettings() const noexcept = 0;
    virtual bool SetHeadsUpDisplayMode(HeadsUpDisplayMode heads_up_display_mode) = 0;

    virtual std::string GetParametersString() = 0;

    virtual ~IApp() = default;
};

} // namespace Methane::UserInterface