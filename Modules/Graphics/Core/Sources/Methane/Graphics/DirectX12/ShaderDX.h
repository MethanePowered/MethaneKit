/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/DirectX12/ShaderDX.h
DirectX 12 implementation of the shader interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/ShaderBase.h>

#include "DescriptorHeapDX.h"

#include <wrl.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <d3d12shader.h>

namespace Methane
{
namespace Graphics
{

namespace wrl = Microsoft::WRL;

class ContextDX;
class ProgramDX;

class ShaderDX final : public ShaderBase
{
public:
    class ResourceBindingDX : public ResourceBindingBase
    {
    public:
        struct Settings
        {
            ResourceBindingBase::Settings base;
            D3D_SHADER_INPUT_TYPE         input_type;
            uint32_t                      count;
            uint32_t                      point;
            uint32_t                      space;
        };

        struct DescriptorRange
        {
            uint32_t             root_parameter_index   = std::numeric_limits<uint32_t>::max();
            DescriptorHeap::Type heap_type              = DescriptorHeap::Type::Undefined;
            uint32_t             offset                 = 0;
            uint32_t             count                  = 0;
        };

        ResourceBindingDX(ContextBase& context, const Settings& settings);
        ResourceBindingDX(const ResourceBindingDX& other) = default;

        // ResourceBinding interface
        void                   SetResource(const Resource::Ptr& sp_resource) override;
        uint32_t               GetResourceCount() const override        { return m_settings_dx.count; }

        const Settings&        GetSettings() const noexcept             { return m_settings_dx; }
        const DescriptorRange& GetDescriptorRange() const noexcept      { return m_descriptor_range; }
        DescriptorHeap::Type   GetDescriptorHeapType() const;

        void SetDescriptorRange(const DescriptorRange& descriptor_range);
        void SetDescriptorHeapReservation(const DescriptorHeap::Reservation* p_descriptor_heap_reservation) 
        { m_p_descriptor_heap_reservation = p_descriptor_heap_reservation; }

    protected:
        ContextDX& GetContextDX();

        const Settings    m_settings_dx;
        DescriptorRange   m_descriptor_range;
        const DescriptorHeap::Reservation* m_p_descriptor_heap_reservation = nullptr;
    };

    ShaderDX(Type type, ContextBase& context, const Settings& settings);

    // ShaderBase
    ResourceBindings GetResourceBindings(const std::set<std::string>& constant_argument_names) const override;

    const wrl::ComPtr<ID3DBlob>& GetNativeByteCode() const noexcept { return m_cp_byte_code; }
    std::vector<D3D12_INPUT_ELEMENT_DESC> GetNativeProgramInputLayout(const ProgramDX& program) const;

protected:
    ContextDX& GetContextDX();

    wrl::ComPtr<ID3DBlob>               m_cp_byte_code;
    wrl::ComPtr<ID3D12ShaderReflection> m_cp_reflection;
};

} // namespace Graphics
} // namespace Methane
