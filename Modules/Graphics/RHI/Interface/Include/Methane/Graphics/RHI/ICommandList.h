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
#include "IResource.h"

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
};

enum class CommandListState
{
    Pending,
    Encoding,
    Committed,
    Executing,
};

struct ICommandListDebugGroup : virtual IObject // NOSONAR
{
    [[nodiscard]] static Ptr<ICommandListDebugGroup> Create(const std::string& name);

    virtual ICommandListDebugGroup& AddSubGroup(Data::Index id, const std::string& name) = 0;
    [[nodiscard]] virtual ICommandListDebugGroup* GetSubGroup(Data::Index id) const noexcept = 0;
    [[nodiscard]] virtual bool HasSubGroups() const noexcept = 0;
};

struct ICommandList;

struct ICommandListCallback
{
    virtual void OnCommandListStateChanged(ICommandList&)        { /* does nothing by default */ };
    virtual void OnCommandListExecutionCompleted(ICommandList&)  { /* does nothing by default */ };
    
    virtual ~ICommandListCallback() = default;
};

struct ICommandQueue;

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
    virtual void  Reset(IDebugGroup* p_debug_group = nullptr) = 0;
    virtual void  ResetOnce(IDebugGroup* p_debug_group = nullptr) = 0;
    virtual void  SetProgramBindings(IProgramBindings& program_bindings,
                                     ProgramBindingsApplyBehavior apply_behavior = ProgramBindingsApplyBehavior(~0U)) = 0;
    virtual void  SetResourceBarriers(const IResourceBarriers& resource_barriers) = 0;
    virtual void  Commit() = 0;
    virtual void  WaitUntilCompleted(uint32_t timeout_ms = 0U) = 0;
    [[nodiscard]] virtual Data::TimeRange GetGpuTimeRange(bool in_cpu_nanoseconds) const = 0;
    [[nodiscard]] virtual ICommandQueue& GetCommandQueue() = 0;
};

struct ICommandListSet
{
    [[nodiscard]] static Ptr<ICommandListSet> Create(const Refs<ICommandList>& command_list_refs, Opt<Data::Index> frame_index_opt = {});

    [[nodiscard]] virtual Data::Size                GetCount() const noexcept = 0;
    [[nodiscard]] virtual const Refs<ICommandList>& GetRefs() const noexcept = 0;
    [[nodiscard]] virtual ICommandList&             operator[](Data::Index index) const = 0;
    [[nodiscard]] virtual const Opt<Data::Index>&   GetFrameIndex() const noexcept = 0;

    virtual ~ICommandListSet() = default;
};

} // namespace Methane::Graphics::Rhi

#ifdef METHANE_COMMAND_DEBUG_GROUPS_ENABLED

#define META_DEBUG_GROUP_CREATE(/*const std::string& */group_name) \
    Methane::Graphics::Rhi::ICommandListDebugGroup::Create(group_name)

#define META_DEBUG_GROUP_PUSH(/*ICommandList& */cmd_list, /*const std::string& */group_name) \
    { \
        const auto s_local_debug_group = META_DEBUG_GROUP_CREATE(group_name); \
        (cmd_list).PushDebugGroup(*s_local_debug_group); \
    }

#define META_DEBUG_GROUP_POP(/*ICommandList& */cmd_list) \
    (cmd_list).PopDebugGroup()

#else

#define META_DEBUG_GROUP_CREATE(/*const std::string& */group_name) \
    nullptr

#define META_DEBUG_GROUP_PUSH(/*ICommandList& */cmd_list, /*const std::string& */group_name) \
    META_UNUSED(cmd_list); META_UNUSED(group_name)

#define META_DEBUG_GROUP_POP(/*ICommandList& */cmd_list) \
    META_UNUSED(cmd_list)

#endif

#define META_DEBUG_GROUP_CREATE_VAR(variable, /*const std::string& */group_name) \
    META_UNUSED(group_name); \
    static const Methane::Ptr<Methane::Graphics::Rhi::ICommandListDebugGroup> variable = META_DEBUG_GROUP_CREATE(group_name)
