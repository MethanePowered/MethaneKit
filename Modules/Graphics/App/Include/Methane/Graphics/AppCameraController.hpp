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

FILE: Methane/Graphics/AppCameraController.hpp
Action camera controller with keyboard and mouse interactions handling.

******************************************************************************/

#pragma once

#include <Methane/Platform/Input/Controller.h>
#include <Methane/Graphics/ActionCamera.h>

namespace Methane
{
namespace Graphics
{

class AppCameraController : public Platform::Input::Controller
{
public:
    using MouseActionByButton = std::map<Platform::Mouse::Button, ActionCamera::MouseAction>;
    using KeyboardActionByKey = std::map<Platform::Keyboard::Key, ActionCamera::KeyboardAction>;

    inline static const MouseActionByButton default_mouse_actions_by_button = {
        { Platform::Mouse::Button::Left,            ActionCamera::MouseAction::Rotate },
        { Platform::Mouse::Button::VScroll,         ActionCamera::MouseAction::Zoom   },
        { Platform::Mouse::Button::Middle,          ActionCamera::MouseAction::Move   },
    };

    inline static const KeyboardActionByKey default_keyboard_actions_by_key = {
        // Move
        { Platform::Keyboard::Key::W,               ActionCamera::KeyboardAction::MoveForward },
        { Platform::Keyboard::Key::S,               ActionCamera::KeyboardAction::MoveBack    },
        { Platform::Keyboard::Key::A,               ActionCamera::KeyboardAction::MoveLeft    },
        { Platform::Keyboard::Key::D,               ActionCamera::KeyboardAction::MoveRight   },
        { Platform::Keyboard::Key::PageUp,          ActionCamera::KeyboardAction::MoveUp      },
        { Platform::Keyboard::Key::PageDown,        ActionCamera::KeyboardAction::MoveDown    },
        // Rotate
        { Platform::Keyboard::Key::Comma,           ActionCamera::KeyboardAction::RollLeft    },
        { Platform::Keyboard::Key::Period,          ActionCamera::KeyboardAction::RollRight   },
        { Platform::Keyboard::Key::Left,            ActionCamera::KeyboardAction::YawLeft     },
        { Platform::Keyboard::Key::Right,           ActionCamera::KeyboardAction::YawRight    },
        { Platform::Keyboard::Key::Up,              ActionCamera::KeyboardAction::PitchUp     },
        { Platform::Keyboard::Key::Down,            ActionCamera::KeyboardAction::PitchDown   },
        // Zoom
        { Platform::Keyboard::Key::Minus,           ActionCamera::KeyboardAction::ZoomOut     },
        { Platform::Keyboard::Key::KeyPadSubtract,  ActionCamera::KeyboardAction::ZoomOut     },
        { Platform::Keyboard::Key::Equal,           ActionCamera::KeyboardAction::ZoomIn      },
        { Platform::Keyboard::Key::KeyPadEqual,     ActionCamera::KeyboardAction::ZoomIn      },
        // Reset
        { Platform::Keyboard::Key::R,               ActionCamera::KeyboardAction::Reset       },
    };

    AppCameraController(ActionCamera& action_camera,
                        const MouseActionByButton& mouse_actions_by_button = default_mouse_actions_by_button,
                        const KeyboardActionByKey& keyboard_actions_by_key = default_keyboard_actions_by_key)
        : m_action_camera(action_camera)
        , m_mouse_actions_by_button(mouse_actions_by_button)
        , m_keyboard_actions_by_key(keyboard_actions_by_key)
    { }

    // Platform::Input::Controller implementation

    void OnMouseButtonChanged(Platform::Mouse::Button button, Platform::Mouse::ButtonState button_state, const Platform::Mouse::StateChange& state_change) override
    {
        const ActionCamera::MouseAction action = GetMouseActionByButton(button);
        switch (button_state)
        {
        case Platform::Mouse::ButtonState::Pressed:  m_action_camera.OnMousePressed(state_change.current.GetPosition(), action); break;
        case Platform::Mouse::ButtonState::Released: m_action_camera.OnMouseReleased(state_change.current.GetPosition()); break;
        }
    }

    void OnMousePositionChanged(const Platform::Mouse::Position& mouse_position, const Platform::Mouse::StateChange&) override
    {
        m_action_camera.OnMouseDragged(mouse_position);
    }

    void OnMouseScrollChanged(const Platform::Mouse::Scroll& mouse_scroll_delta, const Platform::Mouse::StateChange&) override
    {
        const auto mouse_button_and_delta = Platform::Mouse::GetScrollButtonAndDelta(mouse_scroll_delta);
        const ActionCamera::MouseAction action = GetMouseActionByButton(mouse_button_and_delta.first);
        if (action == ActionCamera::MouseAction::Zoom)
        {
            m_action_camera.OnMouseScrolled(mouse_button_and_delta.second);
        }
    }

    void OnKeyboardChanged(Platform::Keyboard::Key key, Platform::Keyboard::KeyState key_state, const Platform::Keyboard::StateChange&) override
    {
        const ActionCamera::KeyboardAction action = GetKeyboardActionByKey(key);
        switch (key_state)
        {
        case Platform::Keyboard::KeyState::Pressed:  m_action_camera.OnKeyPressed(action); break;
        case Platform::Keyboard::KeyState::Released: m_action_camera.OnKeyReleased(action); break;
        }
    }

private:
    inline ActionCamera::MouseAction GetMouseActionByButton(Platform::Mouse::Button mouse_button) const
    {
        const auto mouse_actions_by_button_it = m_mouse_actions_by_button.find(mouse_button);
        return (mouse_actions_by_button_it != m_mouse_actions_by_button.end())
              ? mouse_actions_by_button_it->second : ActionCamera::MouseAction::None;
    }

    inline ActionCamera::KeyboardAction GetKeyboardActionByKey(Platform::Keyboard::Key key) const
    {
        const auto keyboard_actions_by_key_it = m_keyboard_actions_by_key.find(key);
        return (keyboard_actions_by_key_it != m_keyboard_actions_by_key.end())
              ? keyboard_actions_by_key_it->second : ActionCamera::KeyboardAction::None;
    }

    ActionCamera&             m_action_camera;
    const MouseActionByButton m_mouse_actions_by_button;
    const KeyboardActionByKey m_keyboard_actions_by_key;
};

} // namespace Graphics
} // namespace Methane
