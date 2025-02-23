/******************************************************************************

Copyright 2025 Evgeny Gorodetskiy

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

FILE: Tests/Graphics/RHI/DeviceTest.cpp
Unit-tests of the RHI Device

******************************************************************************/

#include "RhiTestHelpers.hpp"

#include <Methane/Graphics/RHI/Device.h>
#include <Methane/Graphics/Null/Device.h>
#include <Methane/Graphics/RHI/System.h>
#include <Methane/Graphics/Null/System.h>

#include <catch2/catch_test_macros.hpp>

using namespace Methane;
using namespace Methane::Graphics;

TEST_CASE("RHI Device Functions", "[device][rhi]")
{
    const Rhi::DeviceCaps device_caps = Rhi::DeviceCaps()
                                      .SetFeatures(Rhi::DeviceFeatureMask(Rhi::DeviceFeature::PresentToWindow))
                                      .SetRenderQueuesCount(2)
                                      .SetComputeQueuesCount(0);
    
    SECTION("Device Initialization")
    {
        const Rhi::Device device(std::make_shared<Null::Device>("Test GPU", false, device_caps));
        REQUIRE(device.IsInitialized());
        CHECK(device.GetInterfacePtr());
    }

    SECTION("Device Destroyed Callback")
    {
        auto device_ptr = std::make_unique<Rhi::Device>(std::make_shared<Null::Device>("Test GPU", false, device_caps));
        ObjectCallbackTester object_callback_tester(*device_ptr);
        CHECK_FALSE(object_callback_tester.IsObjectDestroyed());
        device_ptr.reset();
        CHECK(object_callback_tester.IsObjectDestroyed());
    }

    const Rhi::Device device = Rhi::System::Get().UpdateGpuDevices(device_caps).front();

    SECTION("Object Name Setup")
    {
        CHECK(device.SetName("My device"));
        CHECK(device.GetName() == "My device");
    }

    SECTION("Object Name Change Callback")
    {
        CHECK(device.SetName("My device"));
        ObjectCallbackTester object_callback_tester(device);
        CHECK(device.SetName("Our device"));
        CHECK(object_callback_tester.IsObjectNameChanged());
        CHECK(object_callback_tester.GetCurObjectName() == "Our device");
        CHECK(object_callback_tester.GetOldObjectName() == "My device");
    }

    SECTION("Object Name Set Unchanged")
    {
        CHECK(device.SetName("My device"));
        ObjectCallbackTester object_callback_tester(device);
        CHECK_FALSE(device.SetName("My device"));
        CHECK_FALSE(object_callback_tester.IsObjectNameChanged());
    }

    SECTION("Check Get Adapter Name")
    {
        CHECK(device.GetAdapterName() == "Test GPU 1");
    }

    SECTION("Check Is Software Adapter")
    {
        CHECK_FALSE(device.IsSoftwareAdapter());
    }

    SECTION("Check Get Capabilities")
    {
        CHECK(device.GetCapabilities() == device_caps);
    }

    SECTION("Check String Conversion")
    {
        CHECK(device.ToString() == "GPU \"Test GPU 1\"");
    }

    SECTION("Device Removal Requested Callback")
    {
        DeviceCallbackTester device_callback_tester(device.GetInterface());
        CHECK_FALSE(device_callback_tester.IsDeviceRemovalRequested());
        CHECK_NOTHROW(dynamic_cast<Null::System&>(Rhi::System::Get().GetInterface()).RequestRemoveDevice(device.GetInterface()));
        CHECK(device_callback_tester.IsDeviceRemovalRequested());
    }

    SECTION("Device Removed Callback")
    {
        DeviceCallbackTester device_callback_tester(device.GetInterface());
        CHECK_FALSE(device_callback_tester.IsDeviceRemoved());
        CHECK_NOTHROW(dynamic_cast<Null::System&>(Rhi::System::Get().GetInterface()).RemoveDevice(device.GetInterface()));
        CHECK(device_callback_tester.IsDeviceRemoved());
    }
}
