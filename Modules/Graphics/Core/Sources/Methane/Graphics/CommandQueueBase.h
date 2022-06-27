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

FILE: Methane/Graphics/CommandQueueBase.h
Base implementation of the command queue interface.

******************************************************************************/

#pragma once

#include "ObjectBase.h"
#include "CommandListBase.h"

#include <Methane/Graphics/CommandQueue.h>
#include <Methane/TracyGpu.hpp>

#include <list>
#include <set>
#include <mutex>

namespace Methane::Graphics
{

class ContextBase;
class RenderContextBase;
class DeviceBase;

class CommandQueueBase
    : public ObjectBase
    , public CommandQueue
{
    friend class CommandListBase;

public:
    CommandQueueBase(const ContextBase& context, CommandList::Type command_lists_type);

    // Object interface
    bool SetName(const std::string& name) override;

    // CommandQueue overrides
    [[nodiscard]] const Context& GetContext() const noexcept final;
    CommandList::Type GetCommandListType() const noexcept final { return m_command_lists_type; }
    void Execute(CommandListSet& command_lists, const CommandList::CompletedCallback& completed_callback = {}) override;

    // CommandQueueBase interface
    virtual TimestampQueryBuffer* GetTimestampQueryBuffer() const noexcept { return nullptr; }

    const ContextBase& GetContextBase() const noexcept     { return m_context; }
    DeviceBase&        GetDeviceBase() const noexcept      { return *m_device_ptr; }
    bool               HasTracyContext() const noexcept    { return !!m_tracy_gpu_context_ptr; }
    Tracy::GpuContext* GetTracyContextPtr() const noexcept { return m_tracy_gpu_context_ptr.get(); }
    Tracy::GpuContext& GetTracyContext() const;

protected:
    void InitializeTracyGpuContext(const Tracy::GpuContext::Settings& tracy_settings);

private:
    const ContextBase&           m_context;
    const Ptr<DeviceBase>        m_device_ptr;
    const CommandList::Type      m_command_lists_type;
    UniquePtr<Tracy::GpuContext> m_tracy_gpu_context_ptr;
};

} // namespace Methane::Graphics
