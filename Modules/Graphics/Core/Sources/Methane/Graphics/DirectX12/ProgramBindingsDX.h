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

class ResourceDX;

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

        ArgumentBindingDX(ContextBase& context, SettingsDX settings);
        ArgumentBindingDX(const ArgumentBindingDX& other);

        // ArgumentBinding interface
        void SetResourceLocations(const Resource::Locations& resource_locations) override;

        const SettingsDX&                   GetSettingsDX() const noexcept            { return m_settings_dx; }
        uint32_t                            GetRootParameterIndex() const noexcept    { return m_root_parameter_index; }
        const DescriptorRange&              GetDescriptorRange() const noexcept       { return m_descriptor_range; }
        const ResourceDX::LocationsDX&      GetResourceLocationsDX() const noexcept   { return m_resource_locations_dx; }

        void SetRootParameterIndex(uint32_t root_parameter_index)                           { m_root_parameter_index = root_parameter_index; }
        void SetDescriptorRange(const DescriptorRange& descriptor_range);
        void SetDescriptorHeapReservation(const DescriptorHeap::Reservation* p_reservation) { m_p_descriptor_heap_reservation = p_reservation; }

    private:
        const SettingsDX                   m_settings_dx;
        uint32_t                           m_root_parameter_index = std::numeric_limits<uint32_t>::max();;
        DescriptorRange                    m_descriptor_range;
        const DescriptorHeap::Reservation* m_p_descriptor_heap_reservation  = nullptr;
        ResourceDX::LocationsDX            m_resource_locations_dx;
    };
    
    ProgramBindingsDX(const Ptr<Program>& sp_program, const ResourceLocationsByArgument& resource_locations_by_argument);
    ProgramBindingsDX(const ProgramBindingsDX& other_program_bindings, const ResourceLocationsByArgument& replace_resource_locations_by_argument);

    void Initialize();

    // ProgramBindings interface
    void CompleteInitialization() override;
    void Apply(CommandList& command_list, ApplyBehavior::Mask apply_behavior) const override;

protected:
    using ApplyArgumentBindingFunc = std::function<void(const Program::Argument&, ArgumentBindingDX&, const DescriptorHeap::Reservation*)>;
    void ForEachArgumentBinding(ApplyArgumentBindingFunc apply_argument_binding) const;
    void CopyDescriptorsToGpu();
};

} // namespace Methane::Graphics
