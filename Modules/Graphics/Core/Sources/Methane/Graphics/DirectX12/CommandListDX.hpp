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
#include <Methane/Graphics/Windows/ErrorHandling.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <wrl.h>
#include <d3d12.h>
#include <pix.h>
#include <nowide/convert.hpp>
#include <fmt/format.h>
#include <magic_enum.hpp>

namespace Methane::Graphics
{

namespace wrl = Microsoft::WRL;

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
        META_CHECK_ARG_NOT_NULL(cp_device);

        ThrowIfFailed(cp_device->CreateCommandAllocator(command_list_type, IID_PPV_ARGS(&m_cp_command_allocator)), cp_device.Get());
        ThrowIfFailed(cp_device->CreateCommandList(0, command_list_type, m_cp_command_allocator.Get(), nullptr, IID_PPV_ARGS(&m_cp_command_list)), cp_device.Get());
        m_cp_command_list.As(&m_cp_command_list_4);


        if (TimestampQueryBuffer* p_query_buffer = GetCommandQueueDX().GetTimestampQueryBuffer();
            p_query_buffer)
        {
            m_begin_timestamp_query_ptr = p_query_buffer->CreateTimestampQuery(*this);
            m_end_timestamp_query_ptr   = p_query_buffer->CreateTimestampQuery(*this);

            // Insert beginning GPU timestamp query
            m_begin_timestamp_query_ptr->InsertTimestamp();
        }

        SetCommandListState(CommandList::State::Encoding);
    }

    // CommandList interface

    void PushDebugGroup(CommandList::DebugGroup& debug_group) final
    {
        META_FUNCTION_TASK();

        CommandListBase::PushDebugGroup(debug_group);
        const std::wstring& group_name = static_cast<DebugGroupDX&>(debug_group).GetWideName();
        PIXBeginEvent(m_cp_command_list.Get(), 0, group_name.c_str());
    }

    void PopDebugGroup() final
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
        m_is_native_committed = true;
    }

    void SetResourceBarriers(const Resource::Barriers& resource_barriers) final
    {
        META_FUNCTION_TASK();
        VerifyEncodingState();
        
        const auto lock_guard = resource_barriers.Lock();
        if (resource_barriers.IsEmpty())
            return;

        META_LOG("{} Command list '{}' SET RESOURCE BARRIERS:\n{}", magic_enum::enum_name(GetType()), GetName(), static_cast<std::string>(resource_barriers));
        META_CHECK_ARG_NOT_NULL(m_cp_command_list);

        const std::vector<D3D12_RESOURCE_BARRIER>& dx_resource_barriers = static_cast<const IResourceDX::BarriersDX&>(resource_barriers).GetNativeResourceBarriers();
        m_cp_command_list->ResourceBarrier(static_cast<UINT>(dx_resource_barriers.size()), dx_resource_barriers.data());
    }

    // CommandList interface

    void Reset(CommandList::DebugGroup* p_debug_group) override
    {
        META_FUNCTION_TASK();
        if (!m_is_native_committed)
            return;

        m_is_native_committed = false;

        const wrl::ComPtr<ID3D12Device>& cp_device = GetCommandQueueDX().GetContextDX().GetDeviceDX().GetNativeDevice();
        ThrowIfFailed(m_cp_command_allocator->Reset(), cp_device.Get());
        ThrowIfFailed(m_cp_command_list->Reset(m_cp_command_allocator.Get(), nullptr), cp_device.Get());

        // Insert beginning GPU timestamp query
        if (m_begin_timestamp_query_ptr)
            m_begin_timestamp_query_ptr->InsertTimestamp();

        CommandListBase::Reset(p_debug_group);
    }

    Data::TimeRange GetGpuTimeRange(bool in_cpu_nanoseconds) const final
    {
        META_FUNCTION_TASK();
        if (m_begin_timestamp_query_ptr && m_end_timestamp_query_ptr)
        {
            META_CHECK_ARG_EQUAL_DESCR(GetState(), CommandListBase::State::Pending, "can not get GPU time range of encoding, executing or not committed command list");
            return in_cpu_nanoseconds
                 ? GetNormalTimeRange(m_begin_timestamp_query_ptr->GetCpuNanoseconds(), m_end_timestamp_query_ptr->GetCpuNanoseconds())
                 : GetNormalTimeRange(m_begin_timestamp_query_ptr->GetGpuTimestamp(),   m_end_timestamp_query_ptr->GetGpuTimestamp());
        }
        return CommandListBase::GetGpuTimeRange(in_cpu_nanoseconds);
    }

    // Object interface

    void SetName(const std::string& name) final
    {
        META_FUNCTION_TASK();

        META_CHECK_ARG_NOT_NULL(m_cp_command_list);
        m_cp_command_list->SetName(nowide::widen(name).c_str());

        META_CHECK_ARG_NOT_NULL(m_cp_command_allocator);
        m_cp_command_allocator->SetName(nowide::widen(fmt::format("{} allocator", name)).c_str());

        CommandListBaseT::SetName(name);
    }

    // ICommandListDX interface

    CommandQueueDX&             GetCommandQueueDX() final                             { return static_cast<CommandQueueDX&>(GetCommandQueueBase()); }
    ID3D12GraphicsCommandList&  GetNativeCommandList() const final
    {
        META_CHECK_ARG_NOT_NULL(m_cp_command_list);
        return *m_cp_command_list.Get();
    }
    ID3D12GraphicsCommandList4* GetNativeCommandList4() const final { return m_cp_command_list_4.Get(); }

protected:
    void ApplyProgramBindings(ProgramBindingsBase& program_bindings, ProgramBindings::ApplyBehavior apply_behavior) final
    {
        // Optimization to skip dynamic_cast required to call Apply method of the ProgramBindingBase implementation
        static_cast<ProgramBindingsDX&>(program_bindings).Apply(*this, CommandListBase::GetProgramBindings().get(), apply_behavior);
    }

    bool IsNativeCommitted() const             { return m_is_native_committed; }
    void SetNativeCommitted(bool is_committed) { m_is_native_committed = is_committed; }

    ID3D12CommandAllocator& GetNativeCommandAllocatorRef()
    {
        META_CHECK_ARG_NOT_NULL(m_cp_command_allocator);
        return *m_cp_command_allocator.Get();
    }

    ID3D12GraphicsCommandList& GetNativeCommandListRef()
    {
        META_CHECK_ARG_NOT_NULL(m_cp_command_list);
        return *m_cp_command_list.Get();
    }

    bool                                  HasBeginTimestampQuery() const noexcept { return !!m_begin_timestamp_query_ptr; }
    TimestampQueryBuffer::TimestampQuery& GetBeginTimestampQuery() const
    {
        META_CHECK_ARG_NOT_NULL(m_begin_timestamp_query_ptr);
        return *m_begin_timestamp_query_ptr;
    }

    bool                                  HasEndTimestampQuery() const noexcept   { return !!m_end_timestamp_query_ptr; }
    TimestampQueryBuffer::TimestampQuery& GetEndTimestampQuery() const
    {
        META_CHECK_ARG_NOT_NULL(m_end_timestamp_query_ptr);
        return m_end_timestamp_query_ptr.get();
    }

private:
    static Data::TimeRange GetNormalTimeRange(Timestamp start, Timestamp end)
    {
        return Data::TimeRange(std::min(start, end), std::max(start, end));
    }

    Ptr<TimestampQueryBuffer::TimestampQuery> m_begin_timestamp_query_ptr;
    Ptr<TimestampQueryBuffer::TimestampQuery> m_end_timestamp_query_ptr;
    wrl::ComPtr<ID3D12CommandAllocator>       m_cp_command_allocator;
    wrl::ComPtr<ID3D12GraphicsCommandList>    m_cp_command_list;
    wrl::ComPtr<ID3D12GraphicsCommandList4>   m_cp_command_list_4;    // extended interface for the same command list (may be unavailable on older Windows)
    bool                                      m_is_native_committed = false;
};

} // namespace Methane::Graphics
