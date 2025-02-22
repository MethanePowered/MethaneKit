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

FILE: Tests/Graphics/RHI/SystemTest.cpp
Unit-tests of the RHI System

******************************************************************************/

#include "RhiTestHelpers.hpp"

#include <Methane/Graphics/RHI/System.h>
#include <Methane/Graphics/RHI/Device.h>

#include <string_view>
#include <catch2/catch_test_macros.hpp>

using namespace Methane;
using namespace Methane::Graphics;

TEST_CASE("RHI System Functions", "[rhi][system]")
{
    
    const std::array<std::string_view, 3> test_device_names = {{ "Test GPU 1", "Test GPU 2", "Test WARP" }};
    const Rhi::DeviceCaps required_device_caps = Rhi::DeviceCaps()
                                               .SetFeatures(Rhi::DeviceFeatureMask(Rhi::DeviceFeature::PresentToWindow))
                                               .SetRenderQueuesCount(2)
                                               .SetComputeQueuesCount(0);
    Rhi::System& system = [&required_device_caps]() -> Rhi::System&
                        {
                            auto& system = Rhi::System::Get();
                            system.UpdateGpuDevices(required_device_caps);
                            return system;
                        }();

    SECTION("Update GPU Devices")
    {
        Rhi::Devices devices;
        CHECK_NOTHROW(devices = system.UpdateGpuDevices(required_device_caps));
        CHECK(devices == system.GetGpuDevices());

        int device_index = 0;
        for(const Rhi::Device& device : devices)
        {
            CHECK(device.GetAdapterName() == test_device_names[device_index]);
            CHECK(device.GetCapabilities() == required_device_caps);
            device_index++;
        }
    }

    SECTION("Get Next GPU Device")
    {
        Rhi::Devices devices;
        CHECK_NOTHROW(devices = system.GetGpuDevices());
        REQUIRE(devices.size() > 1);
        CHECK(devices.front().GetAdapterName() == test_device_names[0]);
        
        Rhi::Device next_device = system.GetNextGpuDevice(devices.front());
        REQUIRE(next_device.IsInitialized());
        CHECK(next_device.GetAdapterName() == test_device_names[1]);
    }

    SECTION("Get Software GPU Device")
    {
        Rhi::Device sw_device = system.GetSoftwareGpuDevice();
        REQUIRE(sw_device.IsInitialized());
        CHECK(sw_device.IsSoftwareAdapter());
        CHECK(sw_device.GetAdapterName() == test_device_names.back());
    }

    SECTION("Get Device Capabilities")
    {
        CHECK(system.GetDeviceCapabilities() == required_device_caps);
    }

    SECTION("String Conversion")
    {
        std::stringstream ss;
        ss << "System graphics devices:";
        for(std::string_view device_name : test_device_names)
        {
            ss << std::endl << "  - GPU \""  << device_name << "\";";
        }
        CHECK(system.ToString() == ss.str());
    }

    SECTION("Check For Changes")
    {
        // Dummy method right now, no logic to check
        CHECK_NOTHROW(system.CheckForChanges());
    }
}