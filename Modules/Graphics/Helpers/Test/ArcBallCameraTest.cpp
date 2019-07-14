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

FILE: Test/ArcBallCameraTest.cpp
Arc-Ball camera unit tests

******************************************************************************/

#include <Methane/Graphics/ArcBallCamera.h>
#include <Methane/Data/Types.h>

#include <cml/mathlib/mathlib.h>

using namespace Methane::Graphics;
using namespace Methane::Data;

static const Point2f             g_test_screen_size     = { 640.f, 480.f };
static const Point2f             g_test_screen_center   = g_test_screen_size / 2.f;
static const Camera::Orientation g_test_orientation     = { { 0.f, 5.f, 10.f }, { 0.f, 5.f, 0.f }, { 0.f, 1.f, 0.f } };
static const float               g_test_radius_ratio    = 0.75f;
static const float               g_test_radius_pixels    = g_test_screen_center.y() * g_test_radius_ratio;
static const Vector3f            g_axis_x               = { 1.f, 0.f, 0.f };
static const Vector3f            g_axis_y               = { 0.f, 1.f, 0.f };
static const Vector3f            g_axis_z               = { 0.f, 0.f, -1.f };
static const float               g_cos_of_45_deg        = std::sqrtf(2) / 2.f;

// Approximate comparison of vectors for test purposes only
static float vectors_equal_epsilon = 0.00001f;
inline bool operator==(const Vector3f& left, const Vector3f& right)
{
    for(int i = 0; i < 3; ++i)
    {
        if (std::fabsf(left[i] - right[i]) > vectors_equal_epsilon)
            return false;
    }
    return true;
}

// NOTE: keep crach header include after comparison overloads to make them work on Clang
#include <catch2/catch.hpp>

inline ArcBallCamera SetupCamera(ArcBallCamera::Pivot pivot, const Camera::Orientation& orientation)
{
    ArcBallCamera camera(pivot);
    camera.Resize(g_test_screen_size.x(), g_test_screen_size.y());
    camera.SetOrientation(orientation);
    camera.SetRadiusRatio(g_test_radius_ratio);
    CHECK(camera.GetRadiusInPixels() == g_test_radius_pixels);
    return camera;
}

inline void CheckOrientation(const Camera::Orientation& actual_orientation, const Camera::Orientation& reference_orientation, float epsilon = 0.00001f)
{
    vectors_equal_epsilon = epsilon;
    CHECK(actual_orientation.aim == reference_orientation.aim);
    CHECK(actual_orientation.eye == reference_orientation.eye);
    CHECK(actual_orientation.up  == reference_orientation.up);
}

inline void TestCameraRotation(ArcBallCamera::Pivot pivot,
                               const Camera::Orientation& initial_orientation,
                               const Point2i& mouse_press_pos, const Point2i& mouse_drag_pos,
                               const Camera::Orientation& rotated_orientation,
                               const float vectors_equality_epsilon = 0.00001f)
{
    ArcBallCamera camera = SetupCamera(pivot, initial_orientation);
    camera.OnMousePressed(mouse_press_pos);
    camera.OnMouseDragged(mouse_drag_pos);
    CheckOrientation(camera.GetOrientation(), rotated_orientation, vectors_equality_epsilon);
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

TEST_CASE("Arc-ball camera rotation around Aim pivot < 90 degrees", "[arcball][aim]")
{
    const ArcBallCamera::Pivot test_pivot = ArcBallCamera::Pivot::Aim;

    SECTION("Rotation 90 degrees")
    {
        SECTION("Around X-axis")
        {
            TestCameraRotation(test_pivot, g_test_orientation,
                static_cast<Point2i>(g_test_screen_center),
                static_cast<Point2i>(g_test_screen_center + Point2f(0.f, g_test_radius_pixels)),
                { { 0.f, 15.f, 0.f }, g_test_orientation.aim, { 0.f, 0.f, -1.f } });
        }

        SECTION("Around Y-axis")
        {
            TestCameraRotation(test_pivot, g_test_orientation,
                static_cast<Point2i>(g_test_screen_center),
                static_cast<Point2i>(g_test_screen_center + Point2f(g_test_radius_pixels, 0.f)),
                { { 10.f, 5.f, 0.f }, g_test_orientation.aim, g_test_orientation.up });
        }

        SECTION("Around Z-axis")
        {
            TestCameraRotation(test_pivot, g_test_orientation,
                Point2i( static_cast<int>(g_test_screen_center.x()), 0),
                Point2i(0, static_cast<int>(g_test_screen_center.y())),
                { g_test_orientation.eye, g_test_orientation.aim, { -1.f, 0.f, 0.f } });
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
            TestCameraRotation(test_pivot, g_test_orientation,
                static_cast<Point2i>(g_test_screen_center),
                static_cast<Point2i>(g_test_screen_center + Point2f(0.f, g_test_radius_pixels * std::sinf(test_angle_rad))),
                RotateOrientation(g_test_orientation, test_pivot, g_axis_x, test_angle_deg), test_equality_epsilon);
        }

        SECTION("Around Y axis")
        {
            TestCameraRotation(test_pivot, g_test_orientation,
                static_cast<Point2i>(g_test_screen_center),
                static_cast<Point2i>(g_test_screen_center - Point2f(g_test_radius_pixels * std::cosf(test_angle_rad), 0.f)),
                RotateOrientation(g_test_orientation, test_pivot, g_axis_y, test_angle_deg), test_equality_epsilon);
        }

        SECTION("Around Z axis")
        {
            TestCameraRotation(test_pivot, g_test_orientation,
                Point2i(static_cast<int>(g_test_screen_center.x()), 0),
                static_cast<Point2i>(g_test_screen_center - Point2f(g_test_radius_pixels * std::cosf(test_angle_rad), g_test_radius_pixels * std::sinf(test_angle_rad))),
                RotateOrientation(g_test_orientation, test_pivot, g_axis_z, test_angle_deg), test_equality_epsilon);
        }
    }
}

TEST_CASE("Arc-ball camera rotation around Aim pivot > 90 degrees", "[arcball][aim]")
{
    const ArcBallCamera::Pivot test_pivot = ArcBallCamera::Pivot::Aim;

    SECTION("Rotation 180 degrees")
    {
        SECTION("Around X-axis")
        {
            TestCameraRotation(test_pivot, g_test_orientation,
                static_cast<Point2i>(g_test_screen_center - Point2f(0.f, g_test_radius_pixels)),
                static_cast<Point2i>(g_test_screen_center + Point2f(0.f, g_test_radius_pixels)),
                { { 0.f, 5.f, -10.f }, g_test_orientation.aim, { 0.f, -1.f, 0.f } });
        }

        SECTION("Around Y-axis")
        {
            TestCameraRotation(test_pivot, g_test_orientation,
                static_cast<Point2i>(g_test_screen_center - Point2f(g_test_radius_pixels, 0.f)),
                static_cast<Point2i>(g_test_screen_center + Point2f(g_test_radius_pixels, 0.f)),
                { { 0.f, 5.f, -10.f }, g_test_orientation.aim, g_test_orientation.up });
        }

        SECTION("Around Z-axis")
        {
            TestCameraRotation(test_pivot, g_test_orientation,
                Point2i(static_cast<int>(g_test_screen_center.x()), 0),
                Point2i(static_cast<int>(g_test_screen_center.x()), static_cast<int>(g_test_screen_size.y())),
                { g_test_orientation.eye, g_test_orientation.aim, { 0.f, -1.f, 0.f } });
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
            TestCameraRotation(test_pivot, g_test_orientation,
                static_cast<Point2i>(g_test_screen_center - Point2f(0.f, g_test_radius_pixels)),
                static_cast<Point2i>(g_test_screen_center + Point2f(0.f, g_test_radius_pixels * std::sinf(test_angle_rad))),
                RotateOrientation(g_test_orientation, test_pivot, g_axis_x, test_angle_deg), test_equality_epsilon);
        }

        SECTION("Around Y axis")
        {
            TestCameraRotation(test_pivot, g_test_orientation,
                static_cast<Point2i>(g_test_screen_center - Point2f(g_test_radius_pixels, 0.f)),
                static_cast<Point2i>(g_test_screen_center - Point2f(g_test_radius_pixels * std::cosf(test_angle_rad), 0.f)),
                RotateOrientation(g_test_orientation, test_pivot, g_axis_y, test_angle_deg), test_equality_epsilon);
        }

        SECTION("Around Z axis")
        {
            TestCameraRotation(test_pivot, g_test_orientation,
                Point2i(static_cast<int>(g_test_screen_size.x()), static_cast<int>(g_test_screen_center.y())),
                Point2i(g_test_screen_center + Point2f(g_test_screen_center.y() * std::cosf(test_angle_rad), -1.f * g_test_screen_center.y() * std::sinf(test_angle_rad))),
                RotateOrientation(g_test_orientation, test_pivot, g_axis_z, test_angle_deg), test_equality_epsilon);
        }
    }
}

TEST_CASE("Arc-ball camera rotation around Eye pivot < 90 degrees", "[arcball][eye]")
{
    const ArcBallCamera::Pivot test_pivot = ArcBallCamera::Pivot::Eye;

    SECTION("Rotation 90 degrees")
    {
        SECTION("Around X-axis")
        {
            TestCameraRotation(test_pivot, g_test_orientation,
                static_cast<Point2i>(g_test_screen_center),
                static_cast<Point2i>(g_test_screen_center + Point2f(0.f, g_test_radius_pixels)),
                { g_test_orientation.eye, { 0.f, -5.f, 10.f }, { 0.f, 0.f, -1.f } });
        }

        SECTION("Around Y-axis")
        {
            TestCameraRotation(test_pivot, g_test_orientation,
                static_cast<Point2i>(g_test_screen_center),
                static_cast<Point2i>(g_test_screen_center + Point2f(g_test_radius_pixels, 0.f)),
                { g_test_orientation.eye, { -10.f, 5.f, 10.f }, g_test_orientation.up });
        }

        SECTION("Around Z-axis")
        {
            TestCameraRotation(test_pivot, g_test_orientation,
                Point2i(static_cast<int>(g_test_screen_center.x()), 0),
                Point2i(0, static_cast<int>(g_test_screen_center.y())),
                { g_test_orientation.eye, g_test_orientation.aim, { -1.f, 0.f, 0.f } });
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
            TestCameraRotation(test_pivot, g_test_orientation,
                static_cast<Point2i>(g_test_screen_center),
                static_cast<Point2i>(g_test_screen_center + Point2f(0.f, g_test_radius_pixels * std::sinf(test_angle_rad))),
                RotateOrientation(g_test_orientation, test_pivot, g_axis_x, test_angle_deg), test_equality_epsilon);
        }

        SECTION("Around Y axis")
        {
            TestCameraRotation(test_pivot, g_test_orientation,
                static_cast<Point2i>(g_test_screen_center),
                static_cast<Point2i>(g_test_screen_center - Point2f(g_test_radius_pixels * std::cosf(test_angle_rad), 0.f)),
                RotateOrientation(g_test_orientation, test_pivot, g_axis_y, test_angle_deg), test_equality_epsilon);
        }

        SECTION("Around Z axis")
        {
            TestCameraRotation(test_pivot, g_test_orientation,
                Point2i(static_cast<int>(g_test_screen_center.x()), 0),
                static_cast<Point2i>(g_test_screen_center - Point2f(g_test_radius_pixels * std::cosf(test_angle_rad), g_test_radius_pixels * std::sinf(test_angle_rad))),
                RotateOrientation(g_test_orientation, test_pivot, g_axis_z, test_angle_deg), test_equality_epsilon);
        }
    }
}

TEST_CASE("Arc-ball camera rotation around Eye pivot > 90 degrees", "[arcball][eye]")
{
    const ArcBallCamera::Pivot test_pivot = ArcBallCamera::Pivot::Eye;

    SECTION("Rotation 180 degrees")
    {
        SECTION("Around X-axis")
        {
            TestCameraRotation(test_pivot, g_test_orientation,
                static_cast<Point2i>(g_test_screen_center - Point2f(0.f, g_test_radius_pixels)),
                static_cast<Point2i>(g_test_screen_center + Point2f(0.f, g_test_radius_pixels)),
                { g_test_orientation.eye, { 0.f, 5.f, 20.f }, { 0.f, 0.f, -1.f } });
        }

        SECTION("Around Y-axis")
        {
            TestCameraRotation(test_pivot, g_test_orientation,
                static_cast<Point2i>(g_test_screen_center - Point2f(g_test_radius_pixels, 0.f)),
                static_cast<Point2i>(g_test_screen_center + Point2f(g_test_radius_pixels, 0.f)),
                { g_test_orientation.eye, { 0.f, 5.f, 20.f }, g_test_orientation.up });
        }

        SECTION("Around Z-axis")
        {
            TestCameraRotation(test_pivot, g_test_orientation,
                Point2i(static_cast<int>(g_test_screen_center.x()), 0),
                Point2i(static_cast<int>(g_test_screen_center.x()), static_cast<int>(g_test_screen_size.y())),
                { g_test_orientation.eye, g_test_orientation.aim, { 0.f, -1.f, 0.f } });
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
            TestCameraRotation(test_pivot, g_test_orientation,
                static_cast<Point2i>(g_test_screen_center - Point2f(0.f, g_test_radius_pixels)),
                static_cast<Point2i>(g_test_screen_center + Point2f(0.f, g_test_radius_pixels * std::sinf(test_angle_rad))),
                RotateOrientation(g_test_orientation, test_pivot, g_axis_x, test_angle_deg), test_equality_epsilon);
        }

        SECTION("Around Y axis")
        {
            TestCameraRotation(test_pivot, g_test_orientation,
                static_cast<Point2i>(g_test_screen_center - Point2f(g_test_radius_pixels, 0.f)),
                static_cast<Point2i>(g_test_screen_center - Point2f(g_test_radius_pixels * std::cosf(test_angle_rad), 0.f)),
                RotateOrientation(g_test_orientation, test_pivot, g_axis_y, test_angle_deg), test_equality_epsilon);
        }

        SECTION("Around Z axis")
        {
            TestCameraRotation(test_pivot, g_test_orientation,
                Point2i(static_cast<int>(g_test_screen_size.x()), static_cast<int>(g_test_screen_center.y())),
                Point2i(g_test_screen_center + Point2f(g_test_screen_center.y() * std::cosf(test_angle_rad), -1.f * g_test_screen_center.y() * std::sinf(test_angle_rad))),
                RotateOrientation(g_test_orientation, test_pivot, g_axis_z, test_angle_deg), test_equality_epsilon);
        }
    }
}
