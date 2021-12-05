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

FILE: Methane/Graphics/Metal/ContextMT.hpp
Metal template implementation of the base context interface.

******************************************************************************/

#pragma once

#include "ContextMT.h"
#include "DeviceMT.hh"
#include "ProgramLibraryMT.hh"
#include "DescriptorManagerMT.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Graphics/CommandKit.h>
#include <Methane/Platform/MacOS/Types.hh>
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
    ContextMT(DeviceBase& device, tf::Executor& parallel_executor, const typename ContextBaseT::Settings& settings)
        : ContextBaseT(device, std::make_unique<DescriptorManagerMT>(), parallel_executor, settings)
    {
        META_FUNCTION_TASK();
    }

    ~ContextMT() override
    {
        [m_ns_name release];
    }

    // IContextMT overrides

    const DeviceMT& GetDeviceMT() const noexcept final
    {
        META_FUNCTION_TASK();
        return static_cast<const DeviceMT&>(ContextBase::GetDeviceBase());
    }

    CommandQueueMT& GetDefaultCommandQueueMT(CommandList::Type type) final
    {
        META_FUNCTION_TASK();
        return static_cast<CommandQueueMT&>(ContextBase::GetDefaultCommandKit(type).GetQueue());
    }

    const Ptr<ProgramLibraryMT>& GetLibraryMT(const std::string& library_name) const override
    {
        META_FUNCTION_TASK();
        const auto library_by_name_it = m_library_by_name.find(library_name);
        if (library_by_name_it != m_library_by_name.end())
            return library_by_name_it->second;

        return m_library_by_name.try_emplace(library_name, std::make_shared<ProgramLibraryMT>(GetDeviceMT(), library_name)).first->second;
    }

    // Object overrides

    void SetName(const std::string& name) override
    {
        META_FUNCTION_TASK();
        ContextBase::SetName(name);
        m_ns_name = MacOS::ConvertToNsType<std::string, NSString*>(name);
    }

protected:
    NSString* GetNsName() noexcept { return m_ns_name; }

private:
    using LibraryByName = std::map<std::string, Ptr<ProgramLibraryMT>>;

    mutable LibraryByName m_library_by_name;
    NSString* m_ns_name;
};

} // namespace Methane::Graphics
