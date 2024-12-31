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

FILE: Methane/Graphics/DirectX/Program.h
DirectX 12 implementation of the program interface.

******************************************************************************/

#pragma once

#include "Shader.h"
#include "ProgramBindings.h"

#include <Methane/Graphics/Base/Program.h>

#include <wrl.h>
#include <directx/d3d12.h>

#include <functional>

namespace Methane::Graphics::DirectX
{

struct IContext;
struct IResource;

namespace wrl = Microsoft::WRL;

class Program final : public Base::Program // NOSONAR - this class requires destructor
{
    friend class ProgramBindings;

public:
    Program(Base::Context& context, const Settings& settings);
    ~Program() override;

    // IProgram interface
    [[nodiscard]] Ptr<Rhi::IProgramBindings> CreateBindings(const BindingValueByArgument& binding_value_by_argument, Data::Index frame_index) override;

    // IObject interface
    bool SetName(std::string_view name) override;

    Shader& GetDirectVertexShader() const;
    Shader& GetDirectPixelShader() const;

    const wrl::ComPtr<ID3D12RootSignature>& GetNativeRootSignature() const noexcept { return m_root_signature_cptr; }
    D3D12_INPUT_LAYOUT_DESC                 GetNativeInputLayoutDesc() const noexcept;

    const IContext& GetDirectContext() const noexcept { return m_dx_context; }

private:
    void InitRootSignature();
    DescriptorHeap::Range ReserveDescriptorRange(DescriptorHeap& heap, ArgumentAccessor::Type access_type, uint32_t range_length);

    struct DescriptorHeapReservation
    {
        Ref<DescriptorHeap>   heap;
        DescriptorHeap::Range range;
    };

    using DescriptorRangeByHeapAndAccessType = std::map<std::pair<DescriptorHeap::Type, ArgumentAccessor::Type>, DescriptorHeapReservation>;

    const IContext&                               m_dx_context;
    wrl::ComPtr<ID3D12RootSignature>              m_root_signature_cptr;
    mutable std::vector<D3D12_INPUT_ELEMENT_DESC> m_dx_vertex_input_layout;

    DescriptorRangeByHeapAndAccessType m_constant_descriptor_range_by_heap_and_access_type;
    TracyLockable(std::mutex,          m_constant_descriptor_ranges_reservation_mutex);
};

} // namespace Methane::Graphics::DirectX
