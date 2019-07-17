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

#include <Methane/Platform/InputController.h>
#include <Methane/Graphics/ArcBallCamera.h>

namespace Methane
{
namespace Graphics
{

class AppCameraController : public Platform::InputController
{
public:
    AppCameraController(ArcBallCamera& view_camera, ArcBallCamera* p_light_camera = nullptr)
        : m_view_camera(view_camera)
        , m_p_light_camera(p_light_camera)
    {}

    // InputController
    void OnKeyboardStateChanged(const Platform::Keyboard::State& keyboard_state, const Platform::Keyboard::State& prev_keyboard_state, Platform::Keyboard::State::Property::Mask state_changes_hint) override
    {
        // TODO: not implemented yet
    }

    // InputController
    void OnMouseStateChanged(const Platform::Mouse::State& mouse_state, const Platform::Mouse::State& prev_mouse_state, Platform::Mouse::State::Property::Mask state_changes_hint) override
    {
        if (state_changes_hint & Platform::Mouse::State::Property::Buttons)
        {
            const Platform::Mouse::Buttons pressed_mouse_buttons = mouse_state.GetPressedButtons();
            const Platform::Mouse::Buttons previous_mouse_buttons = prev_mouse_state.GetPressedButtons();

            // Left mouse button is first pressed: initiate Camera rotation
            if (pressed_mouse_buttons.count(Platform::Mouse::Button::Left) &&
                !previous_mouse_buttons.count(Platform::Mouse::Button::Left))
            {
                m_view_camera.OnMousePressed(mouse_state.GetPosition());
            }

            // Right mouse button is first pressed: initiate Light rotation
            if (m_p_light_camera &&
                pressed_mouse_buttons.count(Platform::Mouse::Button::Right) &&
                !previous_mouse_buttons.count(Platform::Mouse::Button::Right))
            {
                m_p_light_camera->OnMousePressed(mouse_state.GetPosition());
            }
        }

        if (state_changes_hint & Platform::Mouse::State::Property::Position)
        {
            const Platform::Mouse::Buttons pressed_mouse_buttons = mouse_state.GetPressedButtons();

            // Mouse is dragged with Left mouse button: rotate Camera
            if (pressed_mouse_buttons.count(Platform::Mouse::Button::Left))
            {
                m_view_camera.OnMouseDragged(mouse_state.GetPosition());
            }

            // Mouse is dragged with Right mouse button: rotate Light
            if (m_p_light_camera &&
                pressed_mouse_buttons.count(Platform::Mouse::Button::Right))
            {
                m_p_light_camera->OnMouseDragged(mouse_state.GetPosition());
            }
        }
    }

private:
    ArcBallCamera& m_view_camera;
    ArcBallCamera* m_p_light_camera;
};

} // namespace Graphics
} // namespace Methane
