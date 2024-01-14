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

FILE: Methane/Graphics/AppCameraController.hpp
Action camera controller with keyboard and mouse interactions handling.

******************************************************************************/

#pragma once

#include <Methane/Platform/Input/Controller.h>
#include <Methane/Platform/Input/KeyboardActionControllerBase.hpp>
#include <Methane/Platform/Input/MouseActionControllerBase.hpp>
#include <Methane/Graphics/ActionCamera.h>

namespace Methane::Graphics
{

namespace pin = Methane::Platform::Input;

class AppCameraController final
    : public pin::Controller
    , public pin::Mouse::ActionControllerBase<ActionCamera::MouseAction>
    , public pin::Keyboard::ActionControllerBase<ActionCamera::KeyboardAction>
{
public:
    inline static const ActionByMouseButton default_actions_by_mouse_button{
        { pin::Mouse::Button::Left,            ActionCamera::MouseAction::Rotate },
        { pin::Mouse::Button::VScroll,         ActionCamera::MouseAction::Zoom   },
        { pin::Mouse::Button::Middle,          ActionCamera::MouseAction::Move   },
    };
    
    inline static const ActionByKeyboardKey default_actions_by_keyboard_key{
        // Move
        { pin::Keyboard::Key::W,               ActionCamera::KeyboardAction::MoveForward },
        { pin::Keyboard::Key::S,               ActionCamera::KeyboardAction::MoveBack    },
        { pin::Keyboard::Key::A,               ActionCamera::KeyboardAction::MoveLeft    },
        { pin::Keyboard::Key::D,               ActionCamera::KeyboardAction::MoveRight   },
        { pin::Keyboard::Key::PageUp,          ActionCamera::KeyboardAction::MoveUp      },
        { pin::Keyboard::Key::PageDown,        ActionCamera::KeyboardAction::MoveDown    },
        // Rotate
        { pin::Keyboard::Key::Comma,           ActionCamera::KeyboardAction::RollLeft    },
        { pin::Keyboard::Key::Period,          ActionCamera::KeyboardAction::RollRight   },
        { pin::Keyboard::Key::Left,            ActionCamera::KeyboardAction::YawLeft     },
        { pin::Keyboard::Key::Right,           ActionCamera::KeyboardAction::YawRight    },
        { pin::Keyboard::Key::Up,              ActionCamera::KeyboardAction::PitchUp     },
        { pin::Keyboard::Key::Down,            ActionCamera::KeyboardAction::PitchDown   },
        // Zoom
        { pin::Keyboard::Key::Minus,           ActionCamera::KeyboardAction::ZoomOut     },
        { pin::Keyboard::Key::KeyPadSubtract,  ActionCamera::KeyboardAction::ZoomOut     },
        { pin::Keyboard::Key::Equal,           ActionCamera::KeyboardAction::ZoomIn      },
        { pin::Keyboard::Key::KeyPadEqual,     ActionCamera::KeyboardAction::ZoomIn      },
    };

    inline static const ActionByKeyboardState default_actions_by_keyboard_state{
        // Other
        { { pin::Keyboard::Key::LeftAlt, pin::Keyboard::Key::R }, ActionCamera::KeyboardAction::Reset       },
        { { pin::Keyboard::Key::LeftAlt, pin::Keyboard::Key::P }, ActionCamera::KeyboardAction::ChangePivot },
    };

    AppCameraController(ActionCamera& action_camera, const std::string& camera_name,
                        const ActionByMouseButton&   mouse_actions_by_button   = default_actions_by_mouse_button,
                        const ActionByKeyboardState& keyboard_actions_by_state = default_actions_by_keyboard_state,
                        const ActionByKeyboardKey&   keyboard_actions_by_key   = default_actions_by_keyboard_key);

    // pin::Controller overrides
    void OnMouseButtonChanged(pin::Mouse::Button button, pin::Mouse::ButtonState button_state, const pin::Mouse::StateChange& state_change) override;
    void OnMousePositionChanged(const pin::Mouse::Position& mouse_position, const pin::Mouse::StateChange&) override;
    void OnMouseScrollChanged(const pin::Mouse::Scroll& mouse_scroll_delta, const pin::Mouse::StateChange&) override;
    void OnKeyboardChanged(pin::Keyboard::Key key, pin::Keyboard::KeyState key_state, const pin::Keyboard::StateChange&) override;
    HelpLines GetHelp() const override;

private:
    // Input::Keyboard::ActionControllerBase interface
    void        OnKeyboardKeyAction(ActionCamera::KeyboardAction, pin::Keyboard::KeyState) override;
    void        OnKeyboardStateAction(ActionCamera::KeyboardAction action) override;
    std::string GetKeyboardActionName(ActionCamera::KeyboardAction action) const override;
    
    // Input::Mouse::ActionControllerBase interface
    std::string GetMouseActionName(ActionCamera::MouseAction action) const override;

    ActionCamera& m_action_camera;
    int m_uninitialized;
    bool m_weirdFlag;
};

} // namespace Methane::Graphics
