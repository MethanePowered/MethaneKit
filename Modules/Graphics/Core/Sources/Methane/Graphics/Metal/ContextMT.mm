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

FILE: Methane/Graphics/Metal/ContextMT.mm
Metal implementation of the context interface.

******************************************************************************/

#include "ContextMT.hh"
#include "DeviceMT.hh"
#include "CommandQueueMT.hh"
#include "TypesMT.hh"

#include <Methane/Instrumentation.h>
#include <Methane/Platform/Utils.h>
#include <Methane/Platform/MacOS/Types.hh>

namespace Methane::Graphics
{

ContextMT::ContextMT(DeviceBase& device, Type type, uint32_t dispatch_count)
    : ContextBase(device, type)
    , m_dispatch_count(dispatch_count)
    , m_dispatch_semaphore(dispatch_semaphore_create(dispatch_count))
{
    ITT_FUNCTION_TASK();

    m_resource_manager.Initialize({ true });
}

ContextMT::~ContextMT()
{
    ITT_FUNCTION_TASK();

    dispatch_release(m_dispatch_semaphore);
}

void ContextMT::Release()
{
    ITT_FUNCTION_TASK();

    // FIXME: semaphore release causes a crash
    // https://stackoverflow.com/questions/8287621/why-does-this-code-cause-exc-bad-instruction
    //dispatch_release(m_dispatch_semaphore);
    
    ContextBase::Release();
}

void ContextMT::Initialize(DeviceBase& device, bool deferred_heap_allocation)
{
    ITT_FUNCTION_TASK();
    
    m_dispatch_semaphore = dispatch_semaphore_create(m_dispatch_count);
    
    ContextBase::Initialize(device, deferred_heap_allocation);
}

void ContextMT::OnCommandQueueCompleted(CommandQueue& /*cmd_queue*/, uint32_t /*frame_index*/)
{
    ITT_FUNCTION_TASK();

    dispatch_semaphore_signal(m_dispatch_semaphore);
}

void ContextMT::WaitForGpu(WaitFor wait_for)
{
    ITT_FUNCTION_TASK();
    
    ContextBase::WaitForGpu(wait_for);
    
    dispatch_semaphore_wait(m_dispatch_semaphore, DISPATCH_TIME_FOREVER);
    
    ContextBase::OnGpuWaitComplete(wait_for);
}

DeviceMT& ContextMT::GetDeviceMT()
{
    ITT_FUNCTION_TASK();
    return static_cast<DeviceMT&>(GetDevice());
}

const Ptr<ContextMT::LibraryMT>& ContextMT::GetLibraryMT(const std::string& library_name)
{
    ITT_FUNCTION_TASK();
    const auto library_by_name_it = m_library_by_name.find(library_name);
    if (library_by_name_it != m_library_by_name.end())
        return library_by_name_it->second;

    return m_library_by_name.emplace(library_name, std::make_shared<LibraryMT>(*this, library_name)).first->second;
}

NSString* ContextMT::LibraryMT::GetFullPath(const std::string& library_name)
{
    return MacOS::ConvertToNSType<std::string, NSString*>(Platform::GetResourceDir() + "/" + library_name + ".metallib");
}

ContextMT::LibraryMT::LibraryMT(ContextMT& metal_context, const std::string& library_name)
        : m_mtl_library(library_name.empty()
                        ? [metal_context.GetDeviceMT().GetNativeDevice() newDefaultLibrary]
                        : [metal_context.GetDeviceMT().GetNativeDevice() newLibraryWithFile:GetFullPath(library_name) error:&m_ns_error])
{
    ITT_FUNCTION_TASK();
    if (!m_mtl_library)
    {
        const std::string error_msg = MacOS::ConvertFromNSType<NSString, std::string>([m_ns_error localizedDescription]);
        throw std::runtime_error("Failed to create " + (library_name.empty() ? std::string("default") : library_name) + " Metal library: " + error_msg);
    }
}

ContextMT::LibraryMT::~LibraryMT()
{
    ITT_FUNCTION_TASK();
    [m_mtl_library release];
}

} // namespace Methane::Graphics
