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

FILE: Methane/Graphics/DirectX12/CommandListDX.hpp
DirectX 12 base template implementation of the command list interface.

******************************************************************************/

#pragma once

#include "CommandListDX.h"
#include "CommandQueueDX.h"
#include "DeviceDX.h"
#include "ContextDX.h"
#include "ResourceDX.h"
#include "QueryBufferDX.h"
#include "ProgramBindingsDX.h"

#include <Methane/Graphics/CommandListBase.h>
#include <Methane/Graphics/Windows/Primitives.h>
#include <Methane/Instrumentation.h>

#include <wrl.h>
#include <d3d12.h>
#include <pix.h>
#include <nowide/convert.hpp>

#include <cassert>

namespace Methane::Graphics
{

namespace wrl = Microsoft::WRL;

class CommandQueueDX;
class RenderPassDX;

template<class CommandListBaseT, typename = std::enable_if_t<std::is_base_of_v<CommandListBase, CommandListBaseT>>>
class CommandListDX
    : public CommandListBaseT
    , public ICommandListDX
{
public:
    template<typename... ConstructArgs>
    explicit CommandListDX(D3D12_COMMAND_LIST_TYPE command_list_type, ConstructArgs&&... construct_args)
        : CommandListBaseT(std::forward<ConstructArgs>(construct_args)...)
    {
        META_FUNCTION_TASK();

        const wrl::ComPtr<ID3D12Device>& cp_device = GetCommandQueueDX().GetContextDX().GetDeviceDX().GetNativeDevice();
        assert(!!cp_device);

        ThrowIfFailed(cp_device->CreateCommandAllocator(command_list_type, IID_PPV_ARGS(&m_cp_command_allocator)), cp_device.Get());
        ThrowIfFailed(cp_device->CreateCommandList(0, command_list_type, m_cp_command_allocator.Get(), nullptr, IID_PPV_ARGS(&m_cp_command_list)), cp_device.Get());
        m_cp_command_list.As(&m_cp_command_list_4);

        TimestampQueryBuffer* p_query_buffer = GetCommandQueueDX().GetTimestampQueryBuffer();
        if (p_query_buffer)
        {
            m_begin_timestamp_query_ptr = p_query_buffer->CreateTimestampQuery(*this);
            m_end_timestamp_query_ptr   = p_query_buffer->CreateTimestampQuery(*this);

            // Insert beginning GPU timestamp query
            m_begin_timestamp_query_ptr->InsertTimestamp();
        }

        SetCommandListState(CommandList::State::Encoding);
    }

    // CommandList interface

    void PushDebugGroup(CommandList::DebugGroup& debug_group) override
    {
        META_FUNCTION_TASK();

        CommandListBase::PushDebugGroup(debug_group);
        const std::wstring& group_name = static_cast<DebugGroupDX&>(debug_group).GetWideName();
        PIXBeginEvent(m_cp_command_list.Get(), 0, group_name.c_str());
    }

    void PopDebugGroup() override
    {
        META_FUNCTION_TASK();

        CommandListBase::PopDebugGroup();
        PIXEndEvent(m_cp_command_list.Get());
    }

    void Commit() override
    {
        META_FUNCTION_TASK();

        CommandListBaseT::Commit();

        // Insert ending GPU timestamp query
        // and resolve timestamps of beginning and ending queries
        if (m_end_timestamp_query_ptr)
        {
            m_end_timestamp_query_ptr->InsertTimestamp();
            m_end_timestamp_query_ptr->ResolveTimestamp();
        }
        if (m_begin_timestamp_query_ptr)
        {
            m_begin_timestamp_query_ptr->ResolveTimestamp();
        }

        m_cp_command_list->Close();
        m_is_committed = true;
    }

    // CommandListBase interface

    void SetResourceBarriers(const ResourceBase::Barriers& resource_barriers) override
    {
        META_FUNCTION_TASK();
        if (resource_barriers.IsEmpty())
            return;

        META_LOG("Command list \"" + GetName() + "\" set resource barriers:\n" + static_cast<std::string>(resource_barriers));

        assert(m_cp_command_list);
        const std::vector<D3D12_RESOURCE_BARRIER>& dx_resource_barriers = static_cast<const ResourceDX::BarriersDX&>(resource_barriers).GetNativeResourceBarriers();
        m_cp_command_list->ResourceBarrier(static_cast<UINT>(dx_resource_barriers.size()), dx_resource_barriers.data());
    }

    // CommandList interface

    void Reset(CommandList::DebugGroup* p_debug_group) override
    {
        META_FUNCTION_TASK();
        if (!m_is_committed)
            return;

        m_is_committed = false;

        const wrl::ComPtr<ID3D12Device>& cp_device = GetCommandQueueDX().GetContextDX().GetDeviceDX().GetNativeDevice();
        ThrowIfFailed(m_cp_command_allocator->Reset(), cp_device.Get());
        ThrowIfFailed(m_cp_command_list->Reset(m_cp_command_allocator.Get(), nullptr), cp_device.Get());

        // Insert beginning GPU timestamp query
        if (m_begin_timestamp_query_ptr)
            m_begin_timestamp_query_ptr->InsertTimestamp();

        CommandListBase::Reset(p_debug_group);
    }

    void SetProgramBindings(ProgramBindings& program_bindings, ProgramBindings::ApplyBehavior::Mask apply_behavior) override
    {
        META_FUNCTION_TASK();
        CommandListBase::CommandState& command_state = CommandListBase::GetCommandState();
        if (command_state.program_bindings_ptr.get() == &program_bindings)
            return;

        ProgramBindingsDX& program_bindings_dx = static_cast<ProgramBindingsDX&>(program_bindings);
        program_bindings_dx.Apply(*this, CommandListBase::GetProgramBindings().get(), apply_behavior);

        Ptr<ObjectBase> program_bindings_object_ptr = program_bindings_dx.GetBasePtr();
        command_state.program_bindings_ptr = std::static_pointer_cast<ProgramBindingsBase>(program_bindings_object_ptr);
        CommandListBase::RetainResource(std::move(program_bindings_object_ptr));
    }

    Data::TimeRange GetGpuTimeRange(bool in_cpu_nanoseconds) const override
    {
        META_FUNCTION_TASK();
        if (m_begin_timestamp_query_ptr && m_end_timestamp_query_ptr)
        {
            if (GetState() != CommandListBase::State::Pending)
                throw std::logic_error("Can not get GPU time range of encoding, executing or not committed command list.");

            return in_cpu_nanoseconds
                 ? Data::TimeRange(m_begin_timestamp_query_ptr->GetCpuNanoseconds(), m_end_timestamp_query_ptr->GetCpuNanoseconds())
                 : Data::TimeRange(m_begin_timestamp_query_ptr->GetGpuTimestamp(),   m_end_timestamp_query_ptr->GetGpuTimestamp());
        }
        return CommandListBase::GetGpuTimeRange(in_cpu_nanoseconds);
    }

    // Object interface

    void SetName(const std::string& name) override
    {
        META_FUNCTION_TASK();

        assert(m_cp_command_list);
        m_cp_command_list->SetName(nowide::widen(name).c_str());

        assert(m_cp_command_allocator);
        m_cp_command_allocator->SetName(nowide::widen(name + " allocator").c_str());

        CommandListBaseT::SetName(name);
    }

    // ICommandListDX interface

    void SetResourceBarriersDX(const ResourceBase::Barriers& resource_barriers) override { SetResourceBarriers(resource_barriers); }
    CommandQueueDX&             GetCommandQueueDX() override           { return static_cast<CommandQueueDX&>(GetCommandQueueBase()); }
    ID3D12GraphicsCommandList&  GetNativeCommandList() const override
    {
        assert(!!m_cp_command_list);
        return *m_cp_command_list.Get();
    }
    ID3D12GraphicsCommandList4* GetNativeCommandList4() const override { return m_cp_command_list_4.Get(); }

protected:
    bool IsCommitted() const             { return m_is_committed; }
    void SetCommitted(bool is_committed) { m_is_committed = is_committed; }

    ID3D12CommandAllocator& GetNativeCommandAllocatorRef()
    {
        assert(!!m_cp_command_allocator);
        return *m_cp_command_allocator.Get();
    }

    ID3D12GraphicsCommandList& GetNativeCommandListRef()
    {
        assert(!!m_cp_command_list);
        return *m_cp_command_list.Get();
    }

    TimestampQueryBuffer::TimestampQuery* GetBeginTimestampQuery() noexcept { return m_begin_timestamp_query_ptr.get(); }
    TimestampQueryBuffer::TimestampQuery* GetEndTimestampQuery() noexcept   { return m_end_timestamp_query_ptr.get(); }

private:
    Ptr<TimestampQueryBuffer::TimestampQuery> m_begin_timestamp_query_ptr;
    Ptr<TimestampQueryBuffer::TimestampQuery> m_end_timestamp_query_ptr;
    wrl::ComPtr<ID3D12CommandAllocator>       m_cp_command_allocator;
    wrl::ComPtr<ID3D12GraphicsCommandList>    m_cp_command_list;
    wrl::ComPtr<ID3D12GraphicsCommandList4>   m_cp_command_list_4;    // extended interface for the same command list (may be unavailable on older Windows)
    bool                                      m_is_committed = false;
};

} // namespace Methane::Graphics
