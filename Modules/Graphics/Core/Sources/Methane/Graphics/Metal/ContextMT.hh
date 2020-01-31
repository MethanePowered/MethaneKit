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

FILE: Methane/Graphics/Metal/ContextMT.hh
Metal implementation of the context interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/ContextBase.h>

#import <Metal/Metal.h>

#include <string>
#include <map>

namespace Methane::Graphics
{

struct CommandQueue;
class DeviceMT;
class CommandQueueMT;

class ContextMT : public virtual ContextBase
{
public:
    class LibraryMT
    {
    public:
        LibraryMT(ContextMT& metal_context, const std::string& library_name = "");
        ~LibraryMT();

        id<MTLLibrary>& Get() noexcept { return m_mtl_library; }

    private:
        static NSString* GetFullPath(const std::string& library_name);

        NSError*       m_ns_error = nil;
        id<MTLLibrary> m_mtl_library;
    };

    ContextMT(DeviceBase& device, Type type, uint32_t dispatch_count);
    ~ContextMT() override;

    // Context interface
    void  WaitForGpu(WaitFor wait_for) override;

    // ContextBase interface
    void  OnCommandQueueCompleted(CommandQueue& cmd_queue, uint32_t frame_index) override;

    DeviceMT&               GetDeviceMT();
    const Ptr<LibraryMT>&   GetLibraryMT(const std::string& library_name = "");

protected:
    // ContextBase overrides
    void Release() override;
    void Initialize(Device& device, bool deferred_heap_allocation) override;

    using LibraryByName = std::map<std::string, Ptr<LibraryMT>>;

    const uint32_t       m_dispatch_count;
    dispatch_semaphore_t m_dispatch_semaphore;
    LibraryByName        m_library_by_name;
};

} // namespace Methane::Graphics
