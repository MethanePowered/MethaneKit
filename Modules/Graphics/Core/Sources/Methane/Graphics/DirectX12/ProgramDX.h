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

FILE: Methane/Graphics/DirectX12/ProgramDX.h
DirectX 12 implementation of the program interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/ProgramBase.h>

#include "ShaderDX.h"

#include <wrl.h>
#include <d3d12.h>
#include <d3dx12.h>

#include <functional>

namespace Methane
{
namespace Graphics
{

class ContextDX;
class ResourceDX;

namespace wrl = Microsoft::WRL;

class ProgramDX final : public ProgramBase
{
public:
    class ResourceBindingsDX : public ResourceBindingsBase
    {
    public:
        ResourceBindingsDX(const Program::Ptr& sp_program, const ResourceByArgument& resource_by_argument);
        ResourceBindingsDX(const ResourceBindingsDX& other_resource_bindings, const ResourceByArgument& replace_resource_by_argument);

        void Initialize();

        // ResourceBindings interface
        void CompleteInitialization() override;
        void Apply(CommandList& command_list) const override;

    protected:
        using ApplyResourceBindingFunc = std::function<void(ResourceDX&, const DescriptorHeap::Reservation&, ShaderDX::ResourceBindingDX& resource_binding)>;
        void ForEachResourceBinding(ApplyResourceBindingFunc apply_resource_binding) const;
        void CopyDescriptorsToGpu();
    };

    ProgramDX(ContextBase& context, const Settings& settings);

    ShaderDX& GetVertexShaderDX() noexcept;
    ShaderDX& GetPixelShaderDX() noexcept;

    const wrl::ComPtr<ID3D12RootSignature>& GetNativeRootSignature() const { return m_dx_root_signature; }
    D3D12_INPUT_LAYOUT_DESC                 GetNativeInputLayoutDesc() const noexcept;

protected:
    void InitRootSignature();

    ContextDX& GetContextDX() noexcept;
    const ContextDX& GetContextDX() const noexcept;

    wrl::ComPtr<ID3D12RootSignature>      m_dx_root_signature;
    std::vector<D3D12_INPUT_ELEMENT_DESC> m_dx_input_layout;
};

} // namespace Graphics
} // namespace Methane
