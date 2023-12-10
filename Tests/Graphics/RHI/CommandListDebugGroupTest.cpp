/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

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

FILE: Tests/Graphics/RHI/CommandListDebugGroupTest.cpp
Unit-tests of the RHI CommandListDebugGroup

******************************************************************************/

#include "RhiTestHelpers.hpp"

#include <Methane/Graphics/RHI/CommandListDebugGroup.h>

#include <memory>

using namespace Methane;
using namespace Methane::Graphics;

TEST_CASE("RHI Command List Debug Group Functions", "[rhi][list][debug]")
{
    SECTION("Command List Debug Group Construction")
    {
        Rhi::CommandListDebugGroup debug_group;
        REQUIRE_NOTHROW(debug_group = Rhi::CommandListDebugGroup("Test"));
        REQUIRE(debug_group.IsInitialized());
        REQUIRE(debug_group.GetInterfacePtr());
        CHECK(debug_group.GetName() == "Test");
        CHECK_FALSE(debug_group.HasSubGroups());
    }

    SECTION("Object Destroyed Callback")
    {
        auto debug_group_ptr = std::make_unique<Rhi::CommandListDebugGroup>("Test");
        ObjectCallbackTester object_callback_tester(*debug_group_ptr);
        CHECK_FALSE(object_callback_tester.IsObjectDestroyed());
        debug_group_ptr.reset();
        CHECK(object_callback_tester.IsObjectDestroyed());
    }

    const Rhi::CommandListDebugGroup debug_group("Test");

    SECTION("Debug Group Can Not be Renamed")
    {
        CHECK_THROWS(debug_group.SetName("Group"));
    }

    SECTION("Can Add Debug Sub-Group")
    {
        CHECK_NOTHROW(debug_group.AddSubGroup(0, "Sub-Group 0"));
        CHECK(debug_group.HasSubGroups());
        Opt<Rhi::CommandListDebugGroup> debug_sub_group_opt;
        REQUIRE_NOTHROW(debug_sub_group_opt = debug_group.GetSubGroup(0));
        REQUIRE(debug_sub_group_opt.has_value());
        CHECK(debug_sub_group_opt->GetName() == "Sub-Group 0");
    }

    SECTION("Can Add Multiple Debug Sub-Groups")
    {
        for(size_t i = 0; i < 10; ++i)
        {
            CHECK_NOTHROW(debug_group.AddSubGroup(i, fmt::format("Sub-Group {}", i)));
        }

        CHECK(debug_group.HasSubGroups());

        for(size_t i = 0; i < 10; ++i)
        {
            Opt<Rhi::CommandListDebugGroup> debug_sub_group_opt;
            REQUIRE_NOTHROW(debug_sub_group_opt = debug_group.GetSubGroup(i));
            REQUIRE(debug_sub_group_opt.has_value());
            CHECK(debug_sub_group_opt->GetName() == fmt::format("Sub-Group {}", i));
        }
    }

    SECTION("Can Not Get Non-Existing Sub-Group")
    {
        CHECK_NOTHROW(debug_group.AddSubGroup(0, "Sub-Group 0"));
        CHECK(debug_group.HasSubGroups());
        Opt<Rhi::CommandListDebugGroup> debug_sub_group_opt;
        REQUIRE_NOTHROW(debug_sub_group_opt = debug_group.GetSubGroup(1));
        REQUIRE_FALSE(debug_sub_group_opt.has_value());
    }
}
