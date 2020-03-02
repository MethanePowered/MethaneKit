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

FILE: Methane/Graphics/Metal/ContextMT.hpp
Metal template implementation of the base context interface.

******************************************************************************/

#pragma once

#include "ContextMT.h"
#include "DeviceMT.hh"
#include "ProgramLibraryMT.hh"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>

#import <Metal/Metal.h>

#include <string>

namespace Methane::Graphics
{

struct CommandQueue;

template<class ContextBaseT, typename = std::enable_if_t<std::is_base_of_v<ContextBase, ContextBaseT>>>
class ContextMT : public ContextBaseT
{
public:
    ContextMT(DeviceBase& device, const typename ContextBaseT::Settings& settings)
        : ContextBaseT(device, settings)
    {
        ITT_FUNCTION_TASK();
        //ContextBase::m_resource_manager.Initialize({ true });
    }

    // IContextMT interface

    DeviceMT& GetDeviceMT() noexcept override
    {
        ITT_FUNCTION_TASK();
        return static_cast<DeviceMT&>(ContextBase::GetDeviceBase());
    }

    CommandQueueMT& GetUploadCommandQueueMT() noexcept override
    {
        ITT_FUNCTION_TASK();
        return static_cast<CommandQueueMT&>(ContextBase::GetUploadCommandQueue());
    }

    const Ptr<ProgramLibraryMT>& GetLibraryMT(const std::string& library_name) override
    {
        ITT_FUNCTION_TASK();
        const auto library_by_name_it = m_library_by_name.find(library_name);
        if (library_by_name_it != m_library_by_name.end())
            return library_by_name_it->second;

        return m_library_by_name.emplace(library_name, std::make_shared<ProgramLibraryMT>(GetDeviceMT(), library_name)).first->second;
    }

private:
    using LibraryByName = std::map<std::string, Ptr<ProgramLibraryMT>>;

    LibraryByName        m_library_by_name;
};

} // namespace Methane::Graphics
