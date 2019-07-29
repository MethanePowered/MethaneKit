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
Arc-ball camera interaction controller via keyboard and mouse actions handling.

******************************************************************************/

#pragma once

#include <Methane/Platform/Input/Controller.h>
#include <Methane/Graphics/ArcBallCamera.h>

namespace Methane
{
namespace Graphics
{

class AppCameraController : public Platform::Input::Controller
{
public:
    using MouseActionByButton = std::map<Platform::Mouse::Button, ArcBallCamera::MouseAction>;
    using KeyboardActionByKey = std::map<Platform::Keyboard::Key, ArcBallCamera::KeyboardAction>;

    inline static const MouseActionByButton default_mouse_actions_by_button = {
        { Platform::Mouse::Button::Left,            ArcBallCamera::MouseAction::Rotate },
        { Platform::Mouse::Button::VScroll,         ArcBallCamera::MouseAction::Zoom   },
        { Platform::Mouse::Button::Middle,          ArcBallCamera::MouseAction::Move   },
    };

    inline static const KeyboardActionByKey default_keyboard_actions_by_key = {
        { Platform::Keyboard::Key::Up,              ArcBallCamera::KeyboardAction::MoveForward },
        { Platform::Keyboard::Key::W,               ArcBallCamera::KeyboardAction::MoveForward },
        { Platform::Keyboard::Key::Down,            ArcBallCamera::KeyboardAction::MoveBack    },
        { Platform::Keyboard::Key::S,               ArcBallCamera::KeyboardAction::MoveBack    },
        { Platform::Keyboard::Key::Left,            ArcBallCamera::KeyboardAction::MoveLeft    },
        { Platform::Keyboard::Key::A,               ArcBallCamera::KeyboardAction::MoveLeft    },
        { Platform::Keyboard::Key::Right,           ArcBallCamera::KeyboardAction::MoveRight   },
        { Platform::Keyboard::Key::D,               ArcBallCamera::KeyboardAction::MoveRight   },
        { Platform::Keyboard::Key::PageUp,          ArcBallCamera::KeyboardAction::MoveUp      },
        { Platform::Keyboard::Key::PageDown,        ArcBallCamera::KeyboardAction::MoveDown    },
        { Platform::Keyboard::Key::Q,               ArcBallCamera::KeyboardAction::RollLeft    },
        { Platform::Keyboard::Key::E,               ArcBallCamera::KeyboardAction::RollRight   },
        { Platform::Keyboard::Key::Minus,           ArcBallCamera::KeyboardAction::ZoomOut     },
        { Platform::Keyboard::Key::KeyPadSubtract,  ArcBallCamera::KeyboardAction::ZoomOut     },
        { Platform::Keyboard::Key::Equal,           ArcBallCamera::KeyboardAction::ZoomIn      },
        { Platform::Keyboard::Key::KeyPadEqual,     ArcBallCamera::KeyboardAction::ZoomIn      },
    };

    AppCameraController(ArcBallCamera& arcball_camera,
                        const MouseActionByButton& mouse_actions_by_button = default_mouse_actions_by_button,
                        const KeyboardActionByKey& keyboard_actions_by_key = default_keyboard_actions_by_key)
        : m_arcball_camera(arcball_camera)
        , m_mouse_actions_by_button(mouse_actions_by_button)
        , m_keyboard_actions_by_key(keyboard_actions_by_key)
    { }

    // Platform::Input::Controller
    void OnMouseButtonChanged(Platform::Mouse::Button button, Platform::Mouse::ButtonState button_state, const Platform::Mouse::StateChange& state_change) override
    {
        const ArcBallCamera::MouseAction action = GetMouseActionByButton(button);
        switch (button_state)
        {
        case Platform::Mouse::ButtonState::Pressed:  m_arcball_camera.OnMousePressed(state_change.current.GetPosition(), action); break;
        case Platform::Mouse::ButtonState::Released: m_arcball_camera.OnMouseReleased(state_change.current.GetPosition()); break;
        }
    }

    // Platform::Input::Controller
    void OnMousePositionChanged(const Platform::Mouse::Position& mouse_position, const Platform::Mouse::StateChange&) override
    {
        m_arcball_camera.OnMouseDragged(mouse_position);
    }

    // Platform::Input::Controller
    void OnMouseScrollChanged(const Platform::Mouse::Scroll& mouse_scroll_delta, const Platform::Mouse::StateChange&) override
    {
        const auto mouse_button_and_delta = Platform::Mouse::GetScrollButtonAndDelta(mouse_scroll_delta);
        const ArcBallCamera::MouseAction action = GetMouseActionByButton(mouse_button_and_delta.first);
        if (action == ArcBallCamera::MouseAction::Zoom)
        {
            m_arcball_camera.OnMouseScrolled(mouse_button_and_delta.second);
        }
    }

    // Platform::Input::Controller
    void OnKeyboardChanged(Platform::Keyboard::Key key, Platform::Keyboard::KeyState key_state, const Platform::Keyboard::StateChange&) override
    {
        const ArcBallCamera::KeyboardAction action = GetKeyboardActionByKey(key);
        switch (key_state)
        {
        case Platform::Keyboard::KeyState::Pressed:  m_arcball_camera.OnKeyPressed(action); break;
        case Platform::Keyboard::KeyState::Released: m_arcball_camera.OnKeyReleased(action); break;
        }
    }

private:
    inline ArcBallCamera::MouseAction GetMouseActionByButton(Platform::Mouse::Button mouse_button) const
    {
        const auto mouse_actions_by_button_it = m_mouse_actions_by_button.find(mouse_button);
        return (mouse_actions_by_button_it != m_mouse_actions_by_button.end())
              ? mouse_actions_by_button_it->second : ArcBallCamera::MouseAction::None;
    }

    inline ArcBallCamera::KeyboardAction GetKeyboardActionByKey(Platform::Keyboard::Key key) const
    {
        const auto keyboard_actions_by_key_it = m_keyboard_actions_by_key.find(key);
        return (keyboard_actions_by_key_it != m_keyboard_actions_by_key.end())
              ? keyboard_actions_by_key_it->second : ArcBallCamera::KeyboardAction::None;
    }

    ArcBallCamera&            m_arcball_camera;
    const MouseActionByButton m_mouse_actions_by_button;
    const KeyboardActionByKey m_keyboard_actions_by_key;
};

} // namespace Graphics
} // namespace Methane
