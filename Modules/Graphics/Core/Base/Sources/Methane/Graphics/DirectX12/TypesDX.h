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

FILE: Methane/Graphics/DirectX12/TypesDX.h
Methane graphics types converters to DirectX 12 native types.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Types.h>
#include <Methane/Graphics/Volume.hpp>

#include <directx/d3dx12.h>
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

    static D3D12_COMPARISON_FUNC CompareFunctionToD3D(Compare compare_func);
    static DXGI_FORMAT PixelFormatToDxgi(const PixelFormat& pixel_format);
    static DXGI_FORMAT PixelFormatToDxgi(const PixelFormat& pixel_format, ResourceFormatType format_type);
    static DXGI_FORMAT ParameterDescToDxgiFormatAndSize(const D3D12_SIGNATURE_PARAMETER_DESC& param_desc, uint32_t& out_element_byte_size);

private:
    TypeConverterDX() = default;
};

} // namespace Methane::Graphics
