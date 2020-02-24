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

FILE: Methane/Graphics/DirectX12/ProgramDX.h
DirectX 12 implementation of the program interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/ProgramBase.h>

#include "ShaderDX.h"

#include <wrl.h>
#include <d3d12.h>

#include <functional>

namespace Methane::Graphics
{

struct IContextDX;
class ResourceDX;

namespace wrl = Microsoft::WRL;

class ProgramDX final : public ProgramBase
{
public:
    ProgramDX(ContextBase& context, const Settings& settings);

    // Object interface
    void SetName(const std::string& name) override;

    ShaderDX& GetVertexShaderDX() noexcept;
    ShaderDX& GetPixelShaderDX() noexcept;

    const wrl::ComPtr<ID3D12RootSignature>& GetNativeRootSignature() const noexcept { return m_cp_root_signature; }
    D3D12_INPUT_LAYOUT_DESC                 GetNativeInputLayoutDesc() const noexcept;

    IContextDX& GetContextDX() noexcept;
    const IContextDX& GetContextDX() const noexcept;

private:
    void InitRootSignature();

    wrl::ComPtr<ID3D12RootSignature>      m_cp_root_signature;
    std::vector<D3D12_INPUT_ELEMENT_DESC> m_dx_input_layout;
};

} // namespace Methane::Graphics
