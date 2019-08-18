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

FILE: Methane/Graphics/AppContextController.h
Graphics context controller for switching parameters in runtime.

******************************************************************************/

#pragma once

#include <Methane/Platform/Input/Controller.h>
#include <Methane/Platform/Keyboard.h>

#include <map>

namespace Methane
{
namespace Graphics
{

struct Context;

class AppContextController : public Platform::Input::Controller
{
public:
    enum class Action : uint32_t
    {
        None = 0,

        SwitchVSync,
        SwitchDevice,

        Count
    };

    using ActionByKeyboardState = std::map<Platform::Keyboard::State, Action>;

    inline static const ActionByKeyboardState default_action_by_keyboard_state = {
        { { Platform::Keyboard::Key::LeftControl, Platform::Keyboard::Key::V }, Action::SwitchVSync  },
        { { Platform::Keyboard::Key::LeftControl, Platform::Keyboard::Key::X }, Action::SwitchDevice },
    };

    AppContextController(Context& context, const ActionByKeyboardState& action_by_keyboard_state = default_action_by_keyboard_state);

    // Input::Controller implementation
    void OnKeyboardChanged(Platform::Keyboard::Key key, Platform::Keyboard::KeyState key_state, const Platform::Keyboard::StateChange&) override;
    HelpLines GetHelp() const override;

    static std::string GetActionName(Action action);

private:
    
    Context&                m_context;
    ActionByKeyboardState   m_action_by_keyboard_state;
};

} // namespace Graphics
} // namespace Methane
