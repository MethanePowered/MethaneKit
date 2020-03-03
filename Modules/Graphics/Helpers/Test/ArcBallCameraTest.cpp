/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Test/ArcBallCameraTest.cpp
Arc-Ball camera unit tests

******************************************************************************/

#include <Methane/Graphics/ArcBallCamera.h>
#include <Methane/Data/Types.h>

#include <catch2/catch.hpp>
#include <cml/mathlib/mathlib.h>
#include <cmath>

using namespace Methane::Graphics;
using namespace Methane::Data;

static const Point2f             g_test_screen_size      = { 640.f, 480.f };
static const Point2f             g_test_screen_center    = g_test_screen_size / 2.f;
static const Camera::Orientation g_test_view_orientation = { { 0.f, 5.f, 10.f }, { 0.f, 5.f, 0.f }, { 0.f, 1.f, 0.f } };
static const Camera::Orientation g_test_dept_orientation = { { 10.f, 7.f, 0.f }, { 0.f, 7.f, 0.f }, { 0.f, 0.f, 1.f } };
static const float               g_test_radius_ratio     = 0.75f;
static const float               g_test_radius_pixels    = g_test_screen_center.GetY() * g_test_radius_ratio;
static const Vector3f            g_axis_x                = { 1.f, 0.f, 0.f };
static const Vector3f            g_axis_y                = { 0.f, 1.f, 0.f };
static const Vector3f            g_axis_z                = { 0.f, 0.f, -1.f };
static float                     g_vectors_equal_epsilon = 0.00001f;

namespace Catch {

// Override vectors comparison with approximate comparison for test purposes
template<>
bool compareEqual(Vector3f const& left, Vector3f const& right)
{
    for (int i = 0; i < 3; ++i)
    {
        if (std::fabs(left[i] - right[i]) > g_vectors_equal_epsilon)
            return false;
    }
    return true;
}

}

inline void SetupCamera(ArcBallCamera& camera, const Camera::Orientation& orientation)
{
    camera.Resize(g_test_screen_size.GetX(), g_test_screen_size.GetY());
    camera.SetOrientation(orientation);
    camera.SetRadiusRatio(g_test_radius_ratio);
    CHECK(camera.GetRadiusInPixels() == g_test_radius_pixels);
}

inline ArcBallCamera SetupViewCamera(ArcBallCamera::Pivot pivot, const Camera::Orientation& orientation)
{
    ArcBallCamera view_camera(pivot);
    SetupCamera(view_camera, orientation);
    return view_camera;
}

inline ArcBallCamera SetupDependentCamera(const ArcBallCamera& view_camera, ArcBallCamera::Pivot pivot, const Camera::Orientation& orientation)
{
    ArcBallCamera dependent_camera(view_camera, pivot);
    SetupCamera(dependent_camera, orientation);
    return dependent_camera;
}

inline void CheckOrientation(const Camera::Orientation& actual_orientation, const Camera::Orientation& reference_orientation, float epsilon = 0.00001f)
{
    g_vectors_equal_epsilon = epsilon;
    CHECK(actual_orientation.aim == reference_orientation.aim);
    CHECK(actual_orientation.eye == reference_orientation.eye);
    CHECK(actual_orientation.up  == reference_orientation.up);
}

inline void TestViewCameraRotation(ArcBallCamera::Pivot view_pivot,
                                   const Camera::Orientation& initial_view_orientation,
                                   const Point2i& mouse_press_pos, const Point2i& mouse_drag_pos,
                                   const Camera::Orientation& rotated_view_orientation,
                                   const float vectors_equality_epsilon = 0.00001f)
{
    ArcBallCamera view_camera = SetupViewCamera(view_pivot, initial_view_orientation);
    view_camera.OnMousePressed(mouse_press_pos);
    view_camera.OnMouseDragged(mouse_drag_pos);
    CheckOrientation(view_camera.GetOrientation(), rotated_view_orientation, vectors_equality_epsilon);
}

inline void TestDependentCameraRotation(ArcBallCamera::Pivot view_pivot,
                                        const Camera::Orientation& initial_view_orientation,
                                        ArcBallCamera::Pivot dependent_pivot,
                                        const Camera::Orientation& initial_dependent_orientation,
                                        const Point2i& mouse_press_pos, const Point2i& mouse_drag_pos,
                                        const Camera::Orientation& rotated_dependent_orientation,
                                        const float vectors_equality_epsilon = 0.00001f)
{
    ArcBallCamera view_camera      = SetupViewCamera(view_pivot, initial_view_orientation);
    ArcBallCamera dependent_camera = SetupDependentCamera(view_camera, dependent_pivot, initial_dependent_orientation);
    dependent_camera.OnMousePressed(mouse_press_pos);
    dependent_camera.OnMouseDragged(mouse_drag_pos);
    CheckOrientation(dependent_camera.GetOrientation(), rotated_dependent_orientation, vectors_equality_epsilon);
}

inline Camera::Orientation RotateOrientation(const Camera::Orientation& orientation, const ArcBallCamera::Pivot pivot, const Vector3f& axis, float angle_degrees)
{
    Matrix33f rotation_matrix;
    cml::matrix_rotation_axis_angle(rotation_matrix, axis.normalize(), cml::rad(angle_degrees));
    const Vector3f look_dir = rotation_matrix * (orientation.aim - orientation.eye);
    const Vector3f up_dir = rotation_matrix * orientation.up;
    switch (pivot)
    {
    case ArcBallCamera::Pivot::Aim:
        return { orientation.aim - look_dir, orientation.aim, up_dir };
    case ArcBallCamera::Pivot::Eye:
        return { orientation.eye, orientation.eye + look_dir, up_dir };
    }
    return { };
}

TEST_CASE("View arc-ball camera rotation around Aim pivot < 90 degrees", "[camera][rotate][aim][view]")
{
    const ArcBallCamera::Pivot test_pivot = ArcBallCamera::Pivot::Aim;

    SECTION("Rotation 90 degrees")
    {
        SECTION("Around X-axis")
        {
            TestViewCameraRotation(test_pivot, g_test_view_orientation,
                static_cast<Point2i>(g_test_screen_center),
                static_cast<Point2i>(g_test_screen_center + Point2f(0.f, g_test_radius_pixels)),
                { { 0.f, 15.f, 0.f }, g_test_view_orientation.aim, { 0.f, 0.f, -1.f } });
        }

        SECTION("Around Y-axis")
        {
            TestViewCameraRotation(test_pivot, g_test_view_orientation,
                static_cast<Point2i>(g_test_screen_center),
                static_cast<Point2i>(g_test_screen_center + Point2f(g_test_radius_pixels, 0.f)),
                { { 10.f, 5.f, 0.f }, g_test_view_orientation.aim, g_test_view_orientation.up });
        }

        SECTION("Around Z-axis")
        {
            TestViewCameraRotation(test_pivot, g_test_view_orientation,
                Point2i(static_cast<int>(g_test_screen_center.GetX()), 0),
                Point2i(0, static_cast<int>(g_test_screen_center.GetY())),
                { g_test_view_orientation.eye, g_test_view_orientation.aim, { -1.f, 0.f, 0.f } });
        }
    }

    SECTION("Rotation 45 degrees")
    {
        // Lower equality precision because of integer screen-space coordinates use
        const float test_equality_epsilon = 0.1f;
        const float test_angle_deg = 45.f;
        const float test_angle_rad = cml::rad(test_angle_deg);

        SECTION("Around X axis")
        {
            TestViewCameraRotation(test_pivot, g_test_view_orientation,
                static_cast<Point2i>(g_test_screen_center),
                static_cast<Point2i>(g_test_screen_center + Point2f(0.f, g_test_radius_pixels * std::sin(test_angle_rad))),
                RotateOrientation(g_test_view_orientation, test_pivot, g_axis_x, test_angle_deg), test_equality_epsilon);
        }

        SECTION("Around Y axis")
        {
            TestViewCameraRotation(test_pivot, g_test_view_orientation,
                static_cast<Point2i>(g_test_screen_center),
                static_cast<Point2i>(g_test_screen_center - Point2f(g_test_radius_pixels * std::cos(test_angle_rad), 0.f)),
                RotateOrientation(g_test_view_orientation, test_pivot, g_axis_y, test_angle_deg), test_equality_epsilon);
        }

        SECTION("Around Z axis")
        {
            TestViewCameraRotation(test_pivot, g_test_view_orientation,
                Point2i(static_cast<int>(g_test_screen_center.GetX()), 0),
                static_cast<Point2i>(g_test_screen_center - Point2f(g_test_radius_pixels * std::cos(test_angle_rad), g_test_radius_pixels * std::sin(test_angle_rad))),
                RotateOrientation(g_test_view_orientation, test_pivot, g_axis_z, test_angle_deg), test_equality_epsilon);
        }
    }
}

TEST_CASE("View arc-ball camera rotation around Aim pivot > 90 degrees", "[camera][rotate][aim][view]")
{
    const ArcBallCamera::Pivot test_pivot = ArcBallCamera::Pivot::Aim;

    SECTION("Rotation 180 degrees")
    {
        SECTION("Around X-axis")
        {
            TestViewCameraRotation(test_pivot, g_test_view_orientation,
                static_cast<Point2i>(g_test_screen_center - Point2f(0.f, g_test_radius_pixels)),
                static_cast<Point2i>(g_test_screen_center + Point2f(0.f, g_test_radius_pixels)),
                { { 0.f, 5.f, -10.f }, g_test_view_orientation.aim, { 0.f, -1.f, 0.f } });
        }

        SECTION("Around Y-axis")
        {
            TestViewCameraRotation(test_pivot, g_test_view_orientation,
                static_cast<Point2i>(g_test_screen_center - Point2f(g_test_radius_pixels, 0.f)),
                static_cast<Point2i>(g_test_screen_center + Point2f(g_test_radius_pixels, 0.f)),
                { { 0.f, 5.f, -10.f }, g_test_view_orientation.aim, g_test_view_orientation.up });
        }

        SECTION("Around Z-axis")
        {
            TestViewCameraRotation(test_pivot, g_test_view_orientation,
                Point2i(static_cast<int>(g_test_screen_center.GetX()), 0),
                Point2i(static_cast<int>(g_test_screen_center.GetX()), static_cast<int>(g_test_screen_size.GetY())),
                { g_test_view_orientation.eye, g_test_view_orientation.aim, { 0.f, -1.f, 0.f } });
        }
    }

    SECTION("Rotation 135 degrees")
    {
        // Lower equality precision because of integer screen-space coordinates use
        const float test_equality_epsilon = 0.1f;
        const float test_angle_deg = 135.f;
        const float test_angle_rad = cml::rad(test_angle_deg);

        SECTION("Around X axis")
        {
            TestViewCameraRotation(test_pivot, g_test_view_orientation,
                static_cast<Point2i>(g_test_screen_center - Point2f(0.f, g_test_radius_pixels)),
                static_cast<Point2i>(g_test_screen_center + Point2f(0.f, g_test_radius_pixels * std::sin(test_angle_rad))),
                RotateOrientation(g_test_view_orientation, test_pivot, g_axis_x, test_angle_deg), test_equality_epsilon);
        }

        SECTION("Around Y axis")
        {
            TestViewCameraRotation(test_pivot, g_test_view_orientation,
                static_cast<Point2i>(g_test_screen_center - Point2f(g_test_radius_pixels, 0.f)),
                static_cast<Point2i>(g_test_screen_center - Point2f(g_test_radius_pixels * std::cos(test_angle_rad), 0.f)),
                RotateOrientation(g_test_view_orientation, test_pivot, g_axis_y, test_angle_deg), test_equality_epsilon);
        }

        SECTION("Around Z axis")
        {
            TestViewCameraRotation(test_pivot, g_test_view_orientation,
                Point2i(static_cast<int>(g_test_screen_size.GetX()), static_cast<int>(g_test_screen_center.GetY())),
                Point2i(g_test_screen_center + Point2f(g_test_screen_center.GetY() * std::cos(test_angle_rad), -1.f * g_test_screen_center.GetY() * std::sin(test_angle_rad))),
                RotateOrientation(g_test_view_orientation, test_pivot, g_axis_z, test_angle_deg), test_equality_epsilon);
        }
    }
}

TEST_CASE("View arc-ball camera rotation around Eye pivot < 90 degrees", "[camera][rotate][eye][view]")
{
    const ArcBallCamera::Pivot test_pivot = ArcBallCamera::Pivot::Eye;

    SECTION("Rotation 90 degrees")
    {
        SECTION("Around X-axis")
        {
            TestViewCameraRotation(test_pivot, g_test_view_orientation,
                static_cast<Point2i>(g_test_screen_center),
                static_cast<Point2i>(g_test_screen_center + Point2f(0.f, g_test_radius_pixels)),
                { g_test_view_orientation.eye, { 0.f, -5.f, 10.f }, { 0.f, 0.f, -1.f } });
        }

        SECTION("Around Y-axis")
        {
            TestViewCameraRotation(test_pivot, g_test_view_orientation,
                static_cast<Point2i>(g_test_screen_center),
                static_cast<Point2i>(g_test_screen_center + Point2f(g_test_radius_pixels, 0.f)),
                { g_test_view_orientation.eye, { -10.f, 5.f, 10.f }, g_test_view_orientation.up });
        }

        SECTION("Around Z-axis")
        {
            TestViewCameraRotation(test_pivot, g_test_view_orientation,
                Point2i(static_cast<int>(g_test_screen_center.GetX()), 0),
                Point2i(0, static_cast<int>(g_test_screen_center.GetY())),
                { g_test_view_orientation.eye, g_test_view_orientation.aim, { -1.f, 0.f, 0.f } });
        }
    }

    SECTION("Rotation 45 degrees")
    {
        // Lower equality precision because of integer screen-space coordinates use
        const float test_equality_epsilon = 0.1f;
        const float test_angle_deg = 45.f;
        const float test_angle_rad = cml::rad(test_angle_deg);

        SECTION("Around X axis")
        {
            TestViewCameraRotation(test_pivot, g_test_view_orientation,
                static_cast<Point2i>(g_test_screen_center),
                static_cast<Point2i>(g_test_screen_center + Point2f(0.f, g_test_radius_pixels * std::sin(test_angle_rad))),
                RotateOrientation(g_test_view_orientation, test_pivot, g_axis_x, test_angle_deg), test_equality_epsilon);
        }

        SECTION("Around Y axis")
        {
            TestViewCameraRotation(test_pivot, g_test_view_orientation,
                static_cast<Point2i>(g_test_screen_center),
                static_cast<Point2i>(g_test_screen_center - Point2f(g_test_radius_pixels * std::cos(test_angle_rad), 0.f)),
                RotateOrientation(g_test_view_orientation, test_pivot, g_axis_y, test_angle_deg), test_equality_epsilon);
        }

        SECTION("Around Z axis")
        {
            TestViewCameraRotation(test_pivot, g_test_view_orientation,
                Point2i(static_cast<int>(g_test_screen_center.GetX()), 0),
                static_cast<Point2i>(g_test_screen_center - Point2f(g_test_radius_pixels * std::cos(test_angle_rad), g_test_radius_pixels * std::sin(test_angle_rad))),
                RotateOrientation(g_test_view_orientation, test_pivot, g_axis_z, test_angle_deg), test_equality_epsilon);
        }
    }
}

TEST_CASE("View arc-ball camera rotation around Eye pivot > 90 degrees", "[camera][rotate][eye][view]")
{
    const ArcBallCamera::Pivot test_pivot = ArcBallCamera::Pivot::Eye;

    SECTION("Rotation 180 degrees")
    {
        SECTION("Around X-axis")
        {
            TestViewCameraRotation(test_pivot, g_test_view_orientation,
                static_cast<Point2i>(g_test_screen_center - Point2f(0.f, g_test_radius_pixels)),
                static_cast<Point2i>(g_test_screen_center + Point2f(0.f, g_test_radius_pixels)),
                { g_test_view_orientation.eye, { 0.f, 5.f, 20.f }, { 0.f, 0.f, -1.f } });
        }

        SECTION("Around Y-axis")
        {
            TestViewCameraRotation(test_pivot, g_test_view_orientation,
                static_cast<Point2i>(g_test_screen_center - Point2f(g_test_radius_pixels, 0.f)),
                static_cast<Point2i>(g_test_screen_center + Point2f(g_test_radius_pixels, 0.f)),
                { g_test_view_orientation.eye, { 0.f, 5.f, 20.f }, g_test_view_orientation.up });
        }

        SECTION("Around Z-axis")
        {
            TestViewCameraRotation(test_pivot, g_test_view_orientation,
                Point2i(static_cast<int>(g_test_screen_center.GetX()), 0),
                Point2i(static_cast<int>(g_test_screen_center.GetX()), static_cast<int>(g_test_screen_size.GetY())),
                { g_test_view_orientation.eye, g_test_view_orientation.aim, { 0.f, -1.f, 0.f } });
        }
    }

    SECTION("Rotation 135 degrees")
    {
        // Lower equality precision because of integer screen-space coordinates use
        const float test_equality_epsilon = 0.1f;
        const float test_angle_deg = 135.f;
        const float test_angle_rad = cml::rad(test_angle_deg);

        SECTION("Around X axis")
        {
            TestViewCameraRotation(test_pivot, g_test_view_orientation,
                static_cast<Point2i>(g_test_screen_center - Point2f(0.f, g_test_radius_pixels)),
                static_cast<Point2i>(g_test_screen_center + Point2f(0.f, g_test_radius_pixels * std::sin(test_angle_rad))),
                RotateOrientation(g_test_view_orientation, test_pivot, g_axis_x, test_angle_deg), test_equality_epsilon);
        }

        SECTION("Around Y axis")
        {
            TestViewCameraRotation(test_pivot, g_test_view_orientation,
                static_cast<Point2i>(g_test_screen_center - Point2f(g_test_radius_pixels, 0.f)),
                static_cast<Point2i>(g_test_screen_center - Point2f(g_test_radius_pixels * std::cos(test_angle_rad), 0.f)),
                RotateOrientation(g_test_view_orientation, test_pivot, g_axis_y, test_angle_deg), test_equality_epsilon);
        }

        SECTION("Around Z axis")
        {
            TestViewCameraRotation(test_pivot, g_test_view_orientation,
                Point2i(static_cast<int>(g_test_screen_size.GetX()), static_cast<int>(g_test_screen_center.GetY())),
                Point2i(g_test_screen_center + Point2f(g_test_screen_center.GetY() * std::cos(test_angle_rad), -1.f * g_test_screen_center.GetY() * std::sin(test_angle_rad))),
                RotateOrientation(g_test_view_orientation, test_pivot, g_axis_z, test_angle_deg), test_equality_epsilon);
        }
    }
}

TEST_CASE("Dependent arc-ball camera rotation around Aim pivot < 90 degrees", "[camera][rotate][aim][dependent]")
{
    const ArcBallCamera::Pivot test_pivot = ArcBallCamera::Pivot::Aim;

    SECTION("Rotation 90 degrees")
    {
        const float test_equality_epsilon = 0.00001f;

        SECTION("Around X-axis")
        {
            TestDependentCameraRotation(test_pivot, g_test_view_orientation, test_pivot, g_test_dept_orientation,
                static_cast<Point2i>(g_test_screen_center),
                static_cast<Point2i>(g_test_screen_center + Point2f(0.f, g_test_radius_pixels)),
                { g_test_dept_orientation.eye, g_test_dept_orientation.aim, { 0.f, 1.f, 0.f } }, test_equality_epsilon);
        }

        SECTION("Around Y-axis")
        {
            TestDependentCameraRotation(test_pivot, g_test_view_orientation, test_pivot, g_test_dept_orientation,
                static_cast<Point2i>(g_test_screen_center),
                static_cast<Point2i>(g_test_screen_center + Point2f(g_test_radius_pixels, 0.f)),
                { { 0.f, 7.f, 10.f }, g_test_dept_orientation.aim, { -1.f, 0.f, 0.f } }, test_equality_epsilon);
        }

        SECTION("Around Z-axis")
        {
            TestDependentCameraRotation(test_pivot, g_test_view_orientation, test_pivot, g_test_dept_orientation,
                Point2i(static_cast<int>(g_test_screen_center.GetX()), 0),
                Point2i(0, static_cast<int>(g_test_screen_center.GetY())),
                { { 0.f, -3.f, 0.f }, g_test_dept_orientation.aim, g_test_dept_orientation.up }, test_equality_epsilon);
        }
    }

    SECTION("Rotation 45 degrees")
    {
        // Lower equality precision because of integer screen-space coordinates use
        const float test_equality_epsilon = 0.1f;
        const float test_angle_deg = 45.f;
        const float test_angle_rad = cml::rad(test_angle_deg);

        SECTION("Around X axis")
        {
            TestDependentCameraRotation(test_pivot, g_test_view_orientation, test_pivot, g_test_dept_orientation,
                static_cast<Point2i>(g_test_screen_center),
                static_cast<Point2i>(g_test_screen_center + Point2f(0.f, g_test_radius_pixels * std::sin(test_angle_rad))),
                RotateOrientation(g_test_dept_orientation, test_pivot, g_axis_x, test_angle_deg), test_equality_epsilon);
        }

        SECTION("Around Y axis")
        {
            TestDependentCameraRotation(test_pivot, g_test_view_orientation, test_pivot, g_test_dept_orientation,
                static_cast<Point2i>(g_test_screen_center),
                static_cast<Point2i>(g_test_screen_center - Point2f(g_test_radius_pixels * std::cos(test_angle_rad), 0.f)),
                RotateOrientation(g_test_dept_orientation, test_pivot, g_axis_y, test_angle_deg), test_equality_epsilon);
        }

        SECTION("Around Z axis")
        {
            TestDependentCameraRotation(test_pivot, g_test_view_orientation, test_pivot, g_test_dept_orientation,
                Point2i(static_cast<int>(g_test_screen_center.GetX()), 0),
                static_cast<Point2i>(g_test_screen_center - Point2f(g_test_radius_pixels * std::cos(test_angle_rad), g_test_radius_pixels * std::sin(test_angle_rad))),
                RotateOrientation(g_test_dept_orientation, test_pivot, g_axis_z, test_angle_deg), test_equality_epsilon);
        }
    }
}
