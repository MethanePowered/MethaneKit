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

FILE: Methane/Graphics/DirectX/ProgramArgumentBinding.h
DirectX 12 implementation of the program argument binding interface.

******************************************************************************/

#pragma once

#include "IResource.h"

#include <Methane/Graphics/Base/ProgramArgumentBinding.h>

#include <wrl.h>
#include <directx/d3d12.h>

namespace Methane::Graphics::Base
{

class Context;

} // namespace Methane::Graphics::Base

namespace Methane::Graphics::DirectX
{

enum class ProgramArgumentBindingType : uint32_t
{
    DescriptorTable = 0,
    ConstantBufferView,
    ShaderResourceView,
    UnorderedAccessView,
    Constant32Bit,
};

struct ProgramArgumentBindingSettings
    : Rhi::ProgramArgumentBindingSettings
{
    ProgramArgumentBindingType type;
    D3D_SHADER_INPUT_TYPE      input_type;
    uint32_t                   point;
    uint32_t                   space;
};

class ProgramArgumentBinding final  // NOSONAR - custom destructor is required
    : public Base::ProgramArgumentBinding
{
public:
    using Type     = ProgramArgumentBindingType;
    using Settings = ProgramArgumentBindingSettings;

    struct DescriptorRange
    {
        DescriptorHeapType heap_type = DescriptorHeapType::Undefined;
        uint32_t           offset    = 0;
        uint32_t           count     = 0;
    };

    ProgramArgumentBinding(const Base::Context& context, const Settings& settings);
    ProgramArgumentBinding(const ProgramArgumentBinding& other);
    ProgramArgumentBinding(ProgramArgumentBinding&&) noexcept = default;
    ~ProgramArgumentBinding() override = default;

    ProgramArgumentBinding& operator=(const ProgramArgumentBinding&) = delete;
    ProgramArgumentBinding& operator=(ProgramArgumentBinding&&) noexcept = default;

    // Base::ProgramArgumentBinding interface
    [[nodiscard]] Ptr<Base::ProgramArgumentBinding> CreateCopy() const override;

    // IArgumentBinding interface
    bool SetResourceViews(const Rhi::ResourceViews& resource_views) override;
    bool SetRootConstant(const Rhi::RootConstant& root_constant) override;

    const Settings&        GetDirectSettings() const noexcept      { return m_settings_dx; }
    uint32_t               GetRootParameterIndex() const noexcept  { return m_root_parameter_index; }
    const DescriptorRange& GetDescriptorRange() const noexcept     { return m_descriptor_range; }
    const ResourceViews&   GetDirectResourceViews() const noexcept { return m_resource_views_dx; }
    DescriptorHeapType     GetDescriptorHeapType() const;

    void SetRootParameterIndex(uint32_t root_parameter_index)      { m_root_parameter_index = root_parameter_index; }
    void SetDescriptorRange(const DescriptorRange& descriptor_range);
    void SetDescriptorHeapReservation(const DescriptorHeapReservation* p_reservation);

private:
    const Settings                   m_settings_dx;
    const Rhi::ResourceUsageMask     m_shader_usage;
    uint32_t                         m_root_parameter_index = std::numeric_limits<uint32_t>::max();;
    DescriptorRange                  m_descriptor_range;
    const DescriptorHeapReservation* m_p_descriptor_heap_reservation = nullptr;
    ResourceViews                    m_resource_views_dx;
    const wrl::ComPtr<ID3D12Device>  m_cp_native_device;
};

} // namespace Methane::Graphics::DirectX
