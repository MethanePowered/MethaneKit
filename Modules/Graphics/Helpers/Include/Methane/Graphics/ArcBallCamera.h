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

FILE: Methane/Graphics/ArcBallCamera.h
Arc-ball camera implementation.

******************************************************************************/

#pragma once

#include "Camera.h"

#include <cml/quaternion.h>

namespace Methane
{
namespace Graphics
{

class ArcBallCamera : public Camera
{
public:
    ArcBallCamera(float radius = 0.75, cml::AxisOrientation axis_orientation = g_axis_orientation);

    void OnMousePressed(const Vector2f& mouse_screen_pos);
    void OnMouseDragged(const Vector2f& mouse_screen_pos);

protected:
    Matrix44f GetScreenToWorldMatrix();
    Vector3f GetSphereProjection(const Vector3f& world_pos);

    float       m_radius;
    Vector3f    m_mouse_pressed_in_world;
    Vector3f    m_mouse_current_in_world;
    Vector3f    m_mouse_pressed_on_sphere;
    Vector3f    m_mouse_current_on_sphere;
    Quaternionf m_previous_quat;
    Quaternionf m_current_quat;
    Quaternionf m_final_quat;
};

} // namespace Graphics
} // namespace Methane
