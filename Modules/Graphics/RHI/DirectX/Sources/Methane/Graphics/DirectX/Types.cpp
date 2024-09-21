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

FILE: Methane/Graphics/DirectX/Mask.h
Methane graphics types converters to DirectX 12 native types.

******************************************************************************/

#include <Methane/Graphics/DirectX/Types.h>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics::DirectX
{

D3D12_COMPARISON_FUNC TypeConverter::CompareFunctionToD3D(Compare compare_func)
{
    META_FUNCTION_TASK();
    switch (compare_func)
    {
    case Compare::Never:        return D3D12_COMPARISON_FUNC_NEVER;
    case Compare::Always:       return D3D12_COMPARISON_FUNC_ALWAYS;
    case Compare::Less:         return D3D12_COMPARISON_FUNC_LESS;
    case Compare::LessEqual:    return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    case Compare::Greater:      return D3D12_COMPARISON_FUNC_GREATER;
    case Compare::GreaterEqual: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    case Compare::Equal:        return D3D12_COMPARISON_FUNC_EQUAL;
    case Compare::NotEqual:     return D3D12_COMPARISON_FUNC_NOT_EQUAL;
    default:                    META_UNEXPECTED_RETURN(compare_func, D3D12_COMPARISON_FUNC_NEVER);
    }
}

DXGI_FORMAT TypeConverter::PixelFormatToDxgi(const PixelFormat& pixel_format)
{
    META_FUNCTION_TASK();
    switch (pixel_format)
    {
    case PixelFormat::Unknown:          return DXGI_FORMAT_UNKNOWN;
    case PixelFormat::RGBA8:            return DXGI_FORMAT_R8G8B8A8_TYPELESS;
    case PixelFormat::RGBA8Unorm:       return DXGI_FORMAT_R8G8B8A8_UNORM;
    case PixelFormat::RGBA8Unorm_sRGB:  return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    case PixelFormat::BGRA8Unorm:       return DXGI_FORMAT_B8G8R8A8_UNORM;
    case PixelFormat::BGRA8Unorm_sRGB:  return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    case PixelFormat::Depth32Float:     return DXGI_FORMAT_D32_FLOAT;
    case PixelFormat::R32Float:         return DXGI_FORMAT_R32_FLOAT;
    case PixelFormat::R32Uint:          return DXGI_FORMAT_R32_UINT;
    case PixelFormat::R32Sint:          return DXGI_FORMAT_R32_SINT;
    case PixelFormat::R16Float:         return DXGI_FORMAT_R16_FLOAT;
    case PixelFormat::R16Uint:          return DXGI_FORMAT_R16_UINT;
    case PixelFormat::R16Sint:          return DXGI_FORMAT_R16_SINT;
    case PixelFormat::R16Unorm:         return DXGI_FORMAT_R16_UNORM;
    case PixelFormat::R16Snorm:         return DXGI_FORMAT_R16_SNORM;
    case PixelFormat::R8Uint:           return DXGI_FORMAT_R8_UINT;
    case PixelFormat::R8Sint:           return DXGI_FORMAT_R8_SINT;
    case PixelFormat::R8Unorm:          return DXGI_FORMAT_R8_UNORM;
    case PixelFormat::R8Snorm:          return DXGI_FORMAT_R8_SNORM;
    case PixelFormat::A8Unorm:          return DXGI_FORMAT_A8_UNORM;
    default:                            META_UNEXPECTED_RETURN(pixel_format, DXGI_FORMAT_UNKNOWN);
    }
}

DXGI_FORMAT TypeConverter::PixelFormatToDxgi(const PixelFormat& pixel_format, ResourceFormatType format_type)
{
    META_FUNCTION_TASK();

    if (pixel_format == PixelFormat::Depth32Float)
    {
        switch (format_type)
        {
        case ResourceFormatType::Resource:  return DXGI_FORMAT_R32_TYPELESS;
        case ResourceFormatType::ViewRead:  return DXGI_FORMAT_R32_FLOAT;
        case ResourceFormatType::ViewWrite: return DXGI_FORMAT_D32_FLOAT;
        default:                            META_UNEXPECTED_RETURN(format_type, DXGI_FORMAT_R32_TYPELESS);
        }
    }

    return PixelFormatToDxgi(pixel_format);
}

DXGI_FORMAT TypeConverter::ParameterDescToDxgiFormatAndSize(const D3D12_SIGNATURE_PARAMETER_DESC& param_desc, uint32_t& out_element_byte_size)
{
    META_FUNCTION_TASK();
    META_CHECK_RANGE(param_desc.Mask, 1, 16);

    const uint32_t component_32bit_byte_size = 4;
    if (param_desc.Mask == 1)
    {
        out_element_byte_size = component_32bit_byte_size;
        if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
            return DXGI_FORMAT_R32_UINT;
        else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
            return DXGI_FORMAT_R32_SINT;
        else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
            return DXGI_FORMAT_R32_FLOAT;
    }
    else if (param_desc.Mask <= 3)
    {
        out_element_byte_size = 2 * component_32bit_byte_size;
        if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
            return DXGI_FORMAT_R32G32_UINT;
        else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
            return DXGI_FORMAT_R32G32_SINT;
        else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
            return DXGI_FORMAT_R32G32_FLOAT;
    }
    else if (param_desc.Mask <= 7)
    {
        out_element_byte_size = 3 * component_32bit_byte_size;
        if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
            return DXGI_FORMAT_R32G32B32_UINT;
        else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
            return DXGI_FORMAT_R32G32B32_SINT;
        else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
            return DXGI_FORMAT_R32G32B32_FLOAT;
    }
    else if (param_desc.Mask <= 15)
    {
        out_element_byte_size = 4 * component_32bit_byte_size;
        if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
            return DXGI_FORMAT_R32G32B32A32_UINT;
        else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
            return DXGI_FORMAT_R32G32B32A32_SINT;
        else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
    }

    out_element_byte_size = 0;
    return DXGI_FORMAT_UNKNOWN;
}

} // namespace Methane::Graphics::DirectX
