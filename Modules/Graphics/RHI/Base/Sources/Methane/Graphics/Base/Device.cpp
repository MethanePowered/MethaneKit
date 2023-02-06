/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Base/Device.cpp
Base implementation of the device interface.

******************************************************************************/

#include <Methane/Graphics/Base/Device.h>
#include <Methane/Graphics/Base/System.h>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <fmt/format.h>

namespace Methane::Graphics::Base
{

Device::Device(const std::string& adapter_name, bool is_software_adapter, const Capabilities& capabilities)
    : m_system_ptr(static_cast<System&>(Rhi::ISystem::Get()).GetPtr()) // NOSONAR
    , m_adapter_name(adapter_name)
    , m_is_software_adapter(is_software_adapter)
    , m_capabilities(capabilities)
{ }

std::string Device::ToString() const
{
    META_FUNCTION_TASK();
    return fmt::format("GPU \"{}\"", GetAdapterName());
}

void Device::OnRemovalRequested()
{
    META_FUNCTION_TASK();
    Data::Emitter<Rhi::IDeviceCallback>::Emit(&Rhi::IDeviceCallback::OnDeviceRemovalRequested, std::ref(*this));
}

void Device::OnRemoved()
{
    META_FUNCTION_TASK();
    Data::Emitter<Rhi::IDeviceCallback>::Emit(&Rhi::IDeviceCallback::OnDeviceRemoved, std::ref(*this));
}

} // namespace Methane::Graphics::Base
