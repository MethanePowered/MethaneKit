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

FILE: Methane/Graphics/RHI/ICommandList.h
Methane command list interface: this is uncreatable common command list interface,
to create instance refer to IRenderCommandList, etc. for specific derived interface.

******************************************************************************/

#pragma once

#include "IObject.h"
#include "IProgramBindings.h"

#include <Methane/Graphics/Types.h>
#include <Methane/Data/TimeRange.hpp>
#include <Methane/Data/IEmitter.h>

#include <string>
#include <functional>

namespace Methane::Graphics::Rhi
{

enum class CommandListType
{
    Transfer,
    Render,
    ParallelRender,
    Compute,

    Count
};

enum class CommandListState
{
    Pending,
    Encoding,
    Committed,
    Executing,
};

struct ICommandList;

struct ICommandListCallback
{
    virtual void OnCommandListStateChanged(ICommandList&)        { /* does nothing by default */ };
    virtual void OnCommandListExecutionCompleted(ICommandList&)  { /* does nothing by default */ };
    
    virtual ~ICommandListCallback() = default;
};

struct ICommandQueue;
struct IResourceBarriers;
struct ICommandListDebugGroup;

struct ICommandList
    : virtual IObject // NOSONAR
    , virtual Data::IEmitter<ICommandListCallback> // NOSONAR
{
    using Type        = CommandListType;
    using State       = CommandListState;
    using IDebugGroup = ICommandListDebugGroup;
    using ICallback   = ICommandListCallback;

    using CompletedCallback = std::function<void(ICommandList& command_list)>;

    // ICommandList interface
    [[nodiscard]] virtual Type  GetType() const noexcept = 0;
    [[nodiscard]] virtual State GetState() const noexcept = 0;
    virtual void  PushDebugGroup(IDebugGroup& debug_group) = 0;
    virtual void  PopDebugGroup() = 0;
    virtual void  Reset(IDebugGroup* debug_group_ptr = nullptr) = 0;
    virtual void  ResetOnce(IDebugGroup* debug_group_ptr = nullptr) = 0;
    virtual void  SetProgramBindings(IProgramBindings& program_bindings,
                                     ProgramBindingsApplyBehaviorMask apply_behavior = ProgramBindingsApplyBehaviorMask(~0U)) = 0;
    virtual void  SetResourceBarriers(const IResourceBarriers& resource_barriers) = 0;
    virtual void  Commit() = 0;
    virtual void  WaitUntilCompleted(uint32_t timeout_ms = 0U) = 0;
    [[nodiscard]] virtual Data::TimeRange GetGpuTimeRange(bool in_cpu_nanoseconds) const = 0;
    [[nodiscard]] virtual ICommandQueue& GetCommandQueue() = 0;
};

} // namespace Methane::Graphics::Rhi
