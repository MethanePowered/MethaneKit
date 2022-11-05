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

FILE: Methane/Graphics/DirectX12/ProgramArgumentBindingDX.h
DirectX 12 implementation of the program argument binding interface.

******************************************************************************/

#pragma once

#include "ResourceDX.h"

#include <Methane/Graphics/ProgramArgumentBindingBase.h>

#include <wrl.h>
#include <directx/d3d12.h>

namespace Methane::Graphics
{

enum class ProgramArgumentBindingTypeDX : uint32_t
{
    DescriptorTable = 0,
    ConstantBufferView,
    ShaderResourceView,
};

struct ProgramArgumentBindingSettingsDX
    : ProgramArgumentBindingSettings
{
    ProgramArgumentBindingTypeDX type;
    D3D_SHADER_INPUT_TYPE        input_type;
    uint32_t                     point;
    uint32_t                     space;
};

class ContextBase;

class ProgramArgumentBindingDX final  // NOSONAR - custom destructor is required
    : public ProgramArgumentBindingBase
{
public:
    using Type = ProgramArgumentBindingTypeDX;
    using SettingsDX = ProgramArgumentBindingSettingsDX;

    struct DescriptorRange
    {
        DescriptorHeapTypeDX heap_type = DescriptorHeapTypeDX::Undefined;
        uint32_t             offset    = 0;
        uint32_t             count     = 0;
    };

    ProgramArgumentBindingDX(const ContextBase& context, const SettingsDX& settings);
    ProgramArgumentBindingDX(const ProgramArgumentBindingDX& other);
    ProgramArgumentBindingDX(ProgramArgumentBindingDX&&) noexcept = default;
    ~ProgramArgumentBindingDX() override = default;

    ProgramArgumentBindingDX& operator=(const ProgramArgumentBindingDX&) = delete;
    ProgramArgumentBindingDX& operator=(ProgramArgumentBindingDX&&) noexcept = default;

    // IArgumentBinding interface
    bool SetResourceViews(const IResource::Views& resource_views) override;

    const SettingsDX&      GetSettingsDX() const noexcept          { return m_settings_dx; }
    uint32_t               GetRootParameterIndex() const noexcept  { return m_root_parameter_index; }
    const DescriptorRange& GetDescriptorRange() const noexcept     { return m_descriptor_range; }
    const ResourceViewsDX& GetResourceViewsDX() const noexcept     { return m_resource_views_dx; }
    DescriptorHeapTypeDX   GetDescriptorHeapType() const;

    void SetRootParameterIndex(uint32_t root_parameter_index)      { m_root_parameter_index = root_parameter_index; }
    void SetDescriptorRange(const DescriptorRange& descriptor_range);
    void SetDescriptorHeapReservation(const DescriptorHeapReservationDX* p_reservation);

private:
    const SettingsDX                   m_settings_dx;
    uint32_t                           m_root_parameter_index = std::numeric_limits<uint32_t>::max();;
    DescriptorRange                    m_descriptor_range;
    const DescriptorHeapReservationDX* m_p_descriptor_heap_reservation = nullptr;
    ResourceViewsDX                    m_resource_views_dx;
    const wrl::ComPtr<ID3D12Device>    m_cp_native_device;
};

} // namespace Methane::Graphics
