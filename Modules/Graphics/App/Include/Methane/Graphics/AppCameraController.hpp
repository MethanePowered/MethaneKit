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
    enum class MouseAction : uint32_t
    {
        None = 0,
        Rotate,
        Zoom,
        Move,
    };

    using MouseActionByButton = std::map<Platform::Mouse::Button, MouseAction>;

    enum class KeyboardAction : uint32_t
    {
        None,

        // Move
        MoveLeft = 0,
        MoveRight,
        MoveForward,
        MoveBack,

        // Rotate
        YawLeft,
        YawRight,
        RollLeft,
        RollRight,
        PitchUp,
        PitchDown,
        
        // Zoom
        ZoomIn,
        ZoomOut,
    };

    using KeyboardActionByKey = std::map<Platform::Keyboard::Key, KeyboardAction>;

    inline static const MouseActionByButton default_mouse_actions_by_button = {
        { Platform::Mouse::Button::Left,    MouseAction::Rotate },
        { Platform::Mouse::Button::VScroll, MouseAction::Zoom   },
        { Platform::Mouse::Button::Middle,  MouseAction::Move   },
    };

    inline static const KeyboardActionByKey default_keyboard_actions_by_key = {
        { Platform::Keyboard::Key::W,      KeyboardAction::MoveForward },
        { Platform::Keyboard::Key::S,      KeyboardAction::MoveBack    },
        { Platform::Keyboard::Key::A,      KeyboardAction::MoveLeft    },
        { Platform::Keyboard::Key::D,      KeyboardAction::MoveRight   },
        { Platform::Keyboard::Key::Q,      KeyboardAction::RollLeft    },
        { Platform::Keyboard::Key::E,      KeyboardAction::RollRight   },
        { Platform::Keyboard::Key::Minus,  KeyboardAction::ZoomIn      },
        { Platform::Keyboard::Key::Equal,  KeyboardAction::ZoomOut     },
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
        if (button_state != Platform::Mouse::ButtonState::Pressed)
            return;

        const MouseAction action = GetMouseActionByButton(button);
        switch (action)
        {
        case MouseAction::Rotate: m_arcball_camera.OnMousePressed(state_change.current.GetPosition()); break;
        case MouseAction::Move: /* TODO: not implemented yet */ break;
        default: break;
        }
    }

    // Platform::Input::Controller
    void OnMousePositionChanged(const Platform::Mouse::Position& mouse_position, const Platform::Mouse::StateChange& state_change) override
    {
        const Platform::Mouse::Buttons pressed_mouse_buttons = state_change.current.GetPressedButtons();
        for (const Platform::Mouse::Button& pressed_mouse_button : pressed_mouse_buttons)
        {
            const MouseAction action = GetMouseActionByButton(pressed_mouse_button);
            switch (action)
            {
            case MouseAction::Rotate: m_arcball_camera.OnMouseDragged(mouse_position); break;
            case MouseAction::Move: /* TODO: not implemented yet */ break;
            default: break;
            }
        }
    }

    // Platform::Input::Controller
    void OnMouseScrollChanged(const Platform::Mouse::Scroll& mouse_scroll_delta, const Platform::Mouse::StateChange& state_change) override
    {
        const auto mouse_button_and_delta = Platform::Mouse::GetScrollButtonAndDelta(mouse_scroll_delta);
        const MouseAction action = GetMouseActionByButton(mouse_button_and_delta.first);
        if (action != MouseAction::Zoom)
            return;

        m_arcball_camera.OnMouseScrolled(mouse_button_and_delta.second);
    }

private:
    inline MouseAction GetMouseActionByButton(Platform::Mouse::Button mouse_button) const
    {
        const auto mouse_actions_by_button_it = m_mouse_actions_by_button.find(mouse_button);
        return (mouse_actions_by_button_it != m_mouse_actions_by_button.end())
              ? mouse_actions_by_button_it->second : MouseAction::None;
    }

    inline KeyboardAction GetKeyboardActionByKey(Platform::Keyboard::Key key) const
    {
        const auto keyboard_actions_by_key_it = m_keyboard_actions_by_key.find(key);
        return (keyboard_actions_by_key_it != m_keyboard_actions_by_key.end())
              ? keyboard_actions_by_key_it->second : KeyboardAction::None;
    }

    ArcBallCamera&                m_arcball_camera;
    const MouseActionByButton     m_mouse_actions_by_button;
    const KeyboardActionByKey     m_keyboard_actions_by_key;
};

} // namespace Graphics
} // namespace Methane
