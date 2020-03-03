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

FILE: Methane/Graphics/DirectX12/TypesDX.h
Methane graphics types converters to DirectX 12 native types.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Types.h>

#include <d3dx12.h>
#include <d3d12shader.h>

#include <vector>

namespace Methane::Graphics
{

class TypeConverterDX
{
public:
    enum class ResourceFormatType
    {
        ResourceBase,
        ViewRead,
        ViewWrite
    };

    static CD3DX12_VIEWPORT                 ViewportToD3D(const Viewport& viewport) noexcept;
    static std::vector<CD3DX12_VIEWPORT>    ViewportsToD3D(const Viewports& viewports) noexcept;
    static CD3DX12_RECT                     ScissorRectToD3D(const ScissorRect& scissor_rect) noexcept;
    static std::vector<CD3DX12_RECT>        ScissorRectsToD3D(const ScissorRects& scissor_rects) noexcept;
    static DXGI_FORMAT                      DataFormatToDXGI(const PixelFormat& data_format) noexcept;
    static DXGI_FORMAT                      DataFormatToDXGI(const PixelFormat& data_format, ResourceFormatType format_type) noexcept;
    static D3D12_COMPARISON_FUNC            CompareFunctionToDX(Compare compare_func) noexcept;
    static DXGI_FORMAT                      ParameterDescToDxgiFormatAndSize(const D3D12_SIGNATURE_PARAMETER_DESC& param_desc, uint32_t& out_element_byte_size) noexcept;

private:
    TypeConverterDX() = default;
};

} // namespace Methane::Graphics
