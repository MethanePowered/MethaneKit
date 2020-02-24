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

FILE: Methane/Graphics/DirectX12/ContextDX.hpp
DirectX 12 base template implementation of the context interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/ContextBase.h>
#include "FenceDX.h"
#include "DeviceDX.h"
#include "CommandQueueDX.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>

#include <wrl.h>
#include <d3d12.h>

namespace Methane::Graphics
{

namespace wrl = Microsoft::WRL;

template<class ContextBaseT, typename = std::enable_if_t<std::is_base_of_v<ContextBase, ContextBaseT>>>
class ContextDX : public ContextBaseT
{
public:
    ContextDX(DeviceBase& device, const typename ContextBaseT::Settings& settings)
        : ContextBaseT(device, settings)
    {
        ITT_FUNCTION_TASK();
    }

    // ContextBase interface

    void Release() override
    {
        ITT_FUNCTION_TASK();
        GetMutableDeviceDX().ReleaseNativeDevice();
        ContextBaseT::Release();
        static_cast<SystemDX&>(System::Get()).ReportLiveObjects();
    }

    // Object interface

    void SetName(const std::string& name) override
    {
        ITT_FUNCTION_TASK();
        ContextBaseT::SetName(name);
        GetDevice().SetName(name + " Device");
    }

    // IContextDX interface

    const DeviceDX& GetDeviceDX() const noexcept override       { return static_cast<const DeviceDX&>(GetDeviceBase()); }
    CommandQueueDX& GetUploadCommandQueueDX() noexcept override { return static_cast<CommandQueueDX&>(GetUploadCommandQueue()); }

protected:
    DeviceDX& GetMutableDeviceDX() noexcept { return static_cast<DeviceDX&>(GetDeviceBase()); }
};

} // namespace Methane::Graphics
