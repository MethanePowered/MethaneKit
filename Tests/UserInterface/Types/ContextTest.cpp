/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy

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

FILE: Tests/UserInterface/Types/ContextTest.cpp
Unit-tests of the User Interface Context

******************************************************************************/

#include "FakeRenderContext.hpp"

#include <Methane/UserInterface/Context.h>

#include <catch2/catch.hpp>

using namespace Methane;
using namespace Methane::Graphics;
using namespace Methane::UserInterface;

TEST_CASE("User Interface Context Unit Conversions", "[ui][context][unit]")
{
    FakeRenderContext      render_context({ }, 2.0, 96);
    UserInterface::Context ui_context(render_context);
}