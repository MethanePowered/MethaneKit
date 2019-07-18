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
    AppCameraController(ArcBallCamera& arcball_camera, Platform::Mouse::Button rotate_mouse_button)
        : m_arcball_camera(arcball_camera)
        , m_rotate_mouse_button(rotate_mouse_button)
    {}

    // Platform::Input::Controller
    void OnMouseButtonChanged(Platform::Mouse::Button button, Platform::Mouse::ButtonState button_state, const Platform::Mouse::StateChange& state_change) override
    {
        if (button == m_rotate_mouse_button &&
            button_state == Platform::Mouse::ButtonState::Pressed)
        {
            // Left mouse button is first pressed: initiate Camera rotation
            m_arcball_camera.OnMousePressed(state_change.current.GetPosition());
        }
    }

    // Platform::Input::Controller
    void OnMousePositionChanged(const Platform::Mouse::Position& mouse_position, const Platform::Mouse::StateChange& state_change) override
    {
        const Platform::Mouse::Buttons pressed_mouse_buttons = state_change.current.GetPressedButtons();
        if (pressed_mouse_buttons.count(m_rotate_mouse_button))
        {
            // Mouse is dragged with Left mouse button: rotate Camera
            m_arcball_camera.OnMouseDragged(mouse_position);
        }
    }

private:
    ArcBallCamera&                m_arcball_camera;
    const Platform::Mouse::Button m_rotate_mouse_button;
};

} // namespace Graphics
} // namespace Methane
