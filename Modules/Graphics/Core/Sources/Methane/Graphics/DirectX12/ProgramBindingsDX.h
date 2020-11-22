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

FILE: Methane/Graphics/DirectX12/ProgramBindingsDX.h
DirectX 12 implementation of the program bindings interface.

******************************************************************************/

#pragma once

#include "ResourceDX.h"

#include <Methane/Graphics/ProgramBindingsBase.h>

#include <wrl.h>
#include <d3d12.h>

#include <functional>

namespace Methane::Graphics
{

class RenderCommandListDX;
struct ICommandListDX;

namespace wrl = Microsoft::WRL;

class ProgramBindingsDX final : public ProgramBindingsBase
{
public:
    class ArgumentBindingDX final : public ArgumentBindingBase
    {
    public:
        enum class Type : uint32_t
        {
            DescriptorTable = 0,
            ConstantBufferView,
            ShaderResourceView,
        };

        struct SettingsDX : Settings
        {
            Type                      type;
            D3D_SHADER_INPUT_TYPE     input_type;
            uint32_t                  point;
            uint32_t                  space;
        };

        struct DescriptorRange
        {
            DescriptorHeap::Type heap_type = DescriptorHeap::Type::Undefined;
            uint32_t             offset    = 0;
            uint32_t             count     = 0;
        };

        ArgumentBindingDX(const ContextBase& context, const SettingsDX& settings);
        ArgumentBindingDX(const ArgumentBindingDX& other);
        ArgumentBindingDX(ArgumentBindingDX&&) noexcept = default;
        ~ArgumentBindingDX() override = default;

        ArgumentBindingDX& operator=(const ArgumentBindingDX&) = delete;
        ArgumentBindingDX& operator=(ArgumentBindingDX&&) noexcept = default;

        // ArgumentBinding interface
        void SetResourceLocations(const Resource::Locations& resource_locations) override;

        const SettingsDX&               GetSettingsDX() const noexcept            { return m_settings_dx; }
        uint32_t                        GetRootParameterIndex() const noexcept    { return m_root_parameter_index; }
        const DescriptorRange&          GetDescriptorRange() const noexcept       { return m_descriptor_range; }
        const IResourceDX::LocationsDX& GetResourceLocationsDX() const noexcept   { return m_resource_locations_dx; }

        void SetRootParameterIndex(uint32_t root_parameter_index)                     { m_root_parameter_index = root_parameter_index; }
        void SetDescriptorRange(const DescriptorRange& descriptor_range);
        void SetDescriptorHeapReservation(const DescriptorHeap::Reservation* p_reservation);

    private:
        const SettingsDX                   m_settings_dx;
        uint32_t                           m_root_parameter_index = std::numeric_limits<uint32_t>::max();;
        DescriptorRange                    m_descriptor_range;
        const DescriptorHeap::Reservation* m_p_descriptor_heap_reservation = nullptr;
        IResourceDX::LocationsDX           m_resource_locations_dx;
    };
    
    ProgramBindingsDX(const Ptr<Program>& program_ptr, const ResourceLocationsByArgument& resource_locations_by_argument);
    ProgramBindingsDX(const ProgramBindingsDX& other_program_bindings, const ResourceLocationsByArgument& replace_resource_locations_by_argument);

    void Initialize();

    // ProgramBindings interface
    void CompleteInitialization() override;
    void Apply(CommandListBase& command_list, ApplyBehavior apply_behavior) const override;

    void Apply(ICommandListDX& command_list_dx, const ProgramBindingsBase* p_applied_program_bindings, ApplyBehavior apply_behavior) const;

private:
    struct RootParameterBinding
    {
        ArgumentBindingDX&          argument_binding;
        uint32_t                    root_parameter_index = 0U;
        D3D12_GPU_DESCRIPTOR_HANDLE base_descriptor      {  };
        D3D12_GPU_VIRTUAL_ADDRESS   gpu_virtual_address  = 0U;
    };

    struct ResourceState
    {
        Ptr<ResourceBase>     resource_ptr;
        ResourceBase::State   state;
    };

    template<typename FuncType> // function void(ArgumentBindingDX&, const DescriptorHeap::Reservation*)
    void ForEachArgumentBinding(FuncType argument_binding_function) const;
    void AddRootParameterBinding(const Program::ArgumentDesc& argument_desc, const RootParameterBinding& root_parameter_binding);
    void AddResourceState(const Program::ArgumentDesc& argument_desc, ResourceState resource_state);
    void UpdateRootParameterBindings();
    void AddRootParameterBindingsForArgument(ArgumentBindingDX& argument_binding, const DescriptorHeap::Reservation* p_heap_reservation);
    bool ApplyResourceStates(bool apply_constant_resource_states) const;
    void ApplyRootParameterBinding(const RootParameterBinding& root_parameter_binding, ID3D12GraphicsCommandList& d3d12_command_list) const;
    void CopyDescriptorsToGpu();
    void CopyDescriptorsToGpuForArgument(const wrl::ComPtr<ID3D12Device>& d3d12_device, ArgumentBindingDX& argument_binding, const DescriptorHeap::Reservation* p_heap_reservation) const;

    using RootParameterBindings = std::vector<RootParameterBinding>;
    RootParameterBindings m_constant_root_parameter_bindings;
    RootParameterBindings m_variadic_root_parameter_bindings;

    using ResourceStates = std::vector<ResourceState>;
    ResourceStates        m_constant_resource_states;
    ResourceStates        m_variadic_resource_states;

    mutable Ptr<ResourceBase::Barriers> m_resource_transition_barriers_ptr;
};

} // namespace Methane::Graphics
