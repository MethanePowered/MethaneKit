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

#include <catch2/catch.hpp>

#include <Methane/Graphics/ArcBallCamera.h>
#include <Methane/Data/Types.h>

using namespace Methane::Graphics;
using namespace Methane::Data;

static const Point2f             g_test_screen_size        = { 640.f, 480.f };
static const Point2f             g_test_screen_center      = g_test_screen_size / 2.f;
static const Camera::Orientation g_test_camera_orientation = { { 0.0f, 5.0f, 10.0f }, { 0.0f, 5.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } };

// Approximate comparison of vectors for test purposes only
template<size_t N>
static bool operator==(const cml::vector<float, cml::fixed<N>>& left, const cml::vector<float, cml::fixed<N>>& right)
{
    const float epsilon = 0.00001f;
    for(int i = 0; i < N; ++i)
    {
        if (std::fabsf(left[i] - right[i]) > epsilon)
            return false;
    }
    return true;
}

TEST_CASE("Arc-ball camera rotation around aim pivot", "[mouse-state]")
{
    SECTION("Rotate 90 degrees horizontally")
    {
        ArcBallCamera camera(ArcBallCamera::Pivot::Aim);
        camera.Resize(g_test_screen_size.x(), g_test_screen_size.y());
        camera.SetOrientation(g_test_camera_orientation);
        camera.OnMousePressed(static_cast<Point2i>(g_test_screen_center));
        camera.OnMouseDragged(static_cast<Point2i>(g_test_screen_center - Point2f(camera.GetRadiusInPixels(), 0.f)));

        static const Camera::Orientation g_rotated_camera_orientation = { { 10.0f, 5.0f, 0.0f }, { 0.0f, 5.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } };
        CHECK(camera.GetOrientation().aim == g_rotated_camera_orientation.aim);
        CHECK(camera.GetOrientation().eye == g_rotated_camera_orientation.eye);
        CHECK(camera.GetOrientation().up  == g_rotated_camera_orientation.up);
    }

    SECTION("Rotate 90 degrees vertically")
    {
        ArcBallCamera camera(ArcBallCamera::Pivot::Aim);
        camera.Resize(g_test_screen_size.x(), g_test_screen_size.y());
        camera.SetOrientation(g_test_camera_orientation);
        camera.OnMousePressed(static_cast<Point2i>(g_test_screen_center));
        camera.OnMouseDragged(static_cast<Point2i>(g_test_screen_center + Point2f(0.f, camera.GetRadiusInPixels())));

        static const Camera::Orientation g_rotated_camera_orientation = { { 0.0f, 15.0f, 0.0f }, { 0.0f, 5.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } };
        CHECK(camera.GetOrientation().aim == g_rotated_camera_orientation.aim);
        CHECK(camera.GetOrientation().eye == g_rotated_camera_orientation.eye);
        CHECK(camera.GetOrientation().up  == g_rotated_camera_orientation.up);
    }
}