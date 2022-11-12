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

FILE: Methane/Graphics/Base/CommandQueue.h
Base implementation of the command queue interface.

******************************************************************************/

#pragma once

#include "Object.h"
#include "CommandList.h"

#include <Methane/Graphics/ICommandQueue.h>
#include <Methane/TracyGpu.hpp>

#include <list>
#include <set>
#include <mutex>

namespace Methane::Graphics::Base
{

class Context;
class RenderContext;
class Device;

class CommandQueue
    : public Object
    , public ICommandQueue
{
    friend class CommandList;

public:
    CommandQueue(const Context& context, CommandListType command_lists_type);

    // IObject interface
    bool SetName(const std::string& name) override;

    // ICommandQueue overrides
    [[nodiscard]] const IContext& GetContext() const noexcept final;
    CommandListType GetCommandListType() const noexcept final { return m_command_lists_type; }
    void Execute(ICommandListSet& command_lists, const ICommandList::CompletedCallback& completed_callback = {}) override;

    // CommandQueue interface
    virtual ITimestampQueryPool* GetTimestampQueryPool() const noexcept { return nullptr; }

    const Context&     GetBaseContext() const noexcept     { return m_context; }
    Device&            GetBaseDevice() const noexcept      { return *m_device_ptr; }
    bool               HasTracyContext() const noexcept    { return !!m_tracy_gpu_context_ptr; }
    Tracy::GpuContext* GetTracyContextPtr() const noexcept { return m_tracy_gpu_context_ptr.get(); }
    Tracy::GpuContext& GetTracyContext() const;

protected:
    void InitializeTracyGpuContext(const Tracy::GpuContext::Settings& tracy_settings);

private:
    const Context&               m_context;
    const Ptr<Device>            m_device_ptr;
    const CommandListType        m_command_lists_type;
    UniquePtr<Tracy::GpuContext> m_tracy_gpu_context_ptr;
};

} // namespace Methane::Graphics::Base
