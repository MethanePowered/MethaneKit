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

FILE: Methane/Graphics/DirectX/ProgramBindings.h
DirectX 12 implementation of the program bindings interface.

******************************************************************************/

#pragma once

#include "ProgramArgumentBinding.h"

#include <Methane/Graphics/Base/ProgramBindings.h>

#include <wrl.h>
#include <directx/d3d12.h>

#include <magic_enum.hpp>
#include <functional>
#include <array>

namespace Methane::Graphics::DirectX
{

class RenderCommandList;
class Program;
struct ICommandList;

namespace wrl = Microsoft::WRL;

class ProgramBindings final // NOSONAR - custom destructor is required
    : public Base::ProgramBindings
{
public:
    using ArgumentBinding = ProgramArgumentBinding;
    
    ProgramBindings(Program& program, const BindingValueByArgument& binding_value_by_argument, Data::Index frame_index);
    ProgramBindings(const ProgramBindings& other_program_bindings, const BindingValueByArgument& replace_resource_views_by_argument, const Opt<Data::Index>& frame_index);
    ~ProgramBindings() override;

    // Base::ProgramBindings overrides
    void Initialize() override;

    // IProgramBindings overrides
    [[nodiscard]] Ptr<Rhi::IProgramBindings> CreateCopy(const BindingValueByArgument& replace_binding_value_by_argument, const Opt<Data::Index>& frame_index) override;
    void CompleteInitialization() override;
    void Apply(Base::CommandList& command_list, ApplyBehaviorMask apply_behavior) const override;

    void Apply(ICommandList& command_list, const Base::ProgramBindings* applied_program_bindings_ptr, ApplyBehaviorMask apply_behavior) const;

private:
    struct RootParameterBinding
    {
        ArgumentBinding&            argument_binding;
        uint32_t                    root_parameter_index = 0U;
        D3D12_GPU_DESCRIPTOR_HANDLE base_descriptor      {  };
        D3D12_GPU_VIRTUAL_ADDRESS   gpu_virtual_address  = 0U;

        template<Rhi::CommandListType command_list_type>
        void Apply(ID3D12GraphicsCommandList& d3d12_command_list) const;
    };

    template<typename FuncType> // function void(ArgumentBinding&, const DescriptorHeap::Reservation*)
    void ForEachArgumentBinding(FuncType argument_binding_function) const;
    void ReserveDescriptorHeapRanges();
    void AddRootParameterBinding(const Rhi::ProgramArgumentAccessor& argument_desc, const RootParameterBinding& root_parameter_binding);
    void UpdateRootParameterBindings();
    void AddRootParameterBindingsForArgument(ArgumentBinding& argument_binding, const DescriptorHeap::Reservation* p_heap_reservation);
    void ApplyRootParameterBindings(Rhi::ProgramArgumentAccessMask access, const ICommandList& command_list,
                                    const Base::ProgramBindings* applied_program_bindings_ptr, bool apply_changes_only) const;
    template<Rhi::CommandListType command_list_type>
    void ApplyRootParameterBindings(Rhi::ProgramArgumentAccessMask access, ID3D12GraphicsCommandList& d3d12_command_list,
                                    const Base::ProgramBindings* applied_program_bindings_ptr, bool apply_changes_only) const;

    void CopyDescriptorsToGpu() const;
    void CopyDescriptorsToGpuForArgument(const wrl::ComPtr<ID3D12Device>& d3d12_device, ArgumentBinding& argument_binding,
                                         const DescriptorHeap::Reservation* p_heap_reservation) const;

    using RootParameterBindings = std::vector<RootParameterBinding>;
    using RootParameterBindingsByAccess = std::array<RootParameterBindings, magic_enum::enum_count<Rhi::ProgramArgumentAccessType>()>;
    RootParameterBindingsByAccess m_root_parameter_bindings_by_access;

    using DescriptorHeapReservationByType = std::array<std::optional<DescriptorHeap::Reservation>, magic_enum::enum_count<DescriptorHeap::Type>() - 1>;
    DescriptorHeapReservationByType m_descriptor_heap_reservations_by_type;
};

class DescriptorsCountByAccess
{
public:
    DescriptorsCountByAccess();

    uint32_t& operator[](Rhi::ProgramArgumentAccessType access_type);
    uint32_t  operator[](Rhi::ProgramArgumentAccessType access_type) const;

private:
    std::array<uint32_t, magic_enum::enum_count<Rhi::ProgramArgumentAccessType>()> m_count_by_access_type;
};

} // namespace Methane::Graphics::DirectX
