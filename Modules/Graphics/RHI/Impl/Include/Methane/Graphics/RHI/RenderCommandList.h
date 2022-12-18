/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/RenderCommandList.h
Methane RenderCommandList PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include "Pimpl.h"

#include <Methane/Graphics/RHI/IRenderCommandList.h>
#include <Methane/Data/Transmitter.hpp>

namespace Methane::Graphics::Rhi
{

class CommandQueue;
class RenderPass;
class CommandListDebugGroup;
class ResourceBarriers;

class RenderCommandList
    : public Data::Transmitter<Rhi::ICommandListCallback>
    , public Data::Transmitter<Rhi::IObjectCallback>
{
public:
    using Primitive   = RenderPrimitive;
    using Type        = CommandListType;
    using State       = CommandListState;
    using DebugGroup  = CommandListDebugGroup;
    using ICallback   = ICommandListCallback;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(RenderCommandList);

    RenderCommandList(const Ptr<IRenderCommandList>& interface_ptr);
    RenderCommandList(IRenderCommandList& interface_ref);
    RenderCommandList(const CommandQueue& command_queue, const RenderPass& render_pass);

    void Init(const CommandQueue& command_queue, const RenderPass& render_pass);
    void Release();

    bool IsInitialized() const META_PIMPL_NOEXCEPT;
    IRenderCommandList& GetInterface() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    bool SetName(std::string_view name) const;
    std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // ICommandList interface methods
    void  PushDebugGroup(DebugGroup& debug_group);
    void  PopDebugGroup();
    void  Reset(DebugGroup* debug_group_ptr = nullptr);
    void  ResetOnce(DebugGroup* debug_group_ptr = nullptr);
    void  SetProgramBindings(IProgramBindings& program_bindings,
                             ProgramBindingsApplyBehaviorMask apply_behavior = ProgramBindingsApplyBehaviorMask(~0U));
    void  SetResourceBarriers(const ResourceBarriers& resource_barriers);
    void  Commit();
    void  WaitUntilCompleted(uint32_t timeout_ms = 0U);
    [[nodiscard]] Data::TimeRange GetGpuTimeRange(bool in_cpu_nanoseconds) const;
    [[nodiscard]] State GetState() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] CommandQueue GetCommandQueue();

    // IRenderCommandList interface methods
    [[nodiscard]] bool IsValidationEnabled() const META_PIMPL_NOEXCEPT;
    void SetValidationEnabled(bool is_validation_enabled);
    [[nodiscard]] RenderPass GetRenderPass() const;
    void ResetWithState(IRenderState& render_state, DebugGroup* debug_group_ptr = nullptr);
    void ResetWithStateOnce(IRenderState& render_state, DebugGroup* debug_group_ptr = nullptr);
    void SetRenderState(IRenderState& render_state, RenderStateGroupMask state_groups = RenderStateGroupMask(~0U));
    void SetViewState(IViewState& view_state);
    bool SetVertexBuffers(IBufferSet& vertex_buffers, bool set_resource_barriers = true);
    bool SetIndexBuffer(IBuffer& index_buffer, bool set_resource_barriers = true);
    void DrawIndexed(Primitive primitive, uint32_t index_count, uint32_t start_index, uint32_t start_vertex,
                     uint32_t instance_count = 1U, uint32_t start_instance = 0U);
    void Draw(Primitive primitive, uint32_t vertex_count, uint32_t start_vertex,
              uint32_t instance_count = 1U, uint32_t start_instance = 0U);

private:
    class Impl;

    RenderCommandList(UniquePtr<Impl>&& impl_ptr);

    UniquePtr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi
