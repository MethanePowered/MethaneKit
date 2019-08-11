/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/AppHelpController.h
Help displaying controller

******************************************************************************/

#pragma once

#include <Methane/Platform/AppBase.h>

namespace Methane
{
namespace Platform
{

class AppHelpController : public Input::Controller
{
public:
    AppHelpController(AppBase& application, const std::string& application_help,
                      Keyboard::Key help_key = Platform::Keyboard::Key::F1,
                      bool show_command_line_help = false);

    // Input::Controller implementation
    void OnKeyboardChanged(Platform::Keyboard::Key key, Platform::Keyboard::KeyState key_state, const Platform::Keyboard::StateChange&) override;

    // Input::IHelpProvider implementation
    HelpLines GetHelp() const override;

private:
    AppBase&            m_application;
    const Keyboard::Key m_help_key;
    const bool          m_show_command_line_help;
};

} // namespace Graphics
} // namespace Methane
