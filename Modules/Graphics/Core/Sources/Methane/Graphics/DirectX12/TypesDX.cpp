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

#include "TypesDX.h"

#include <Methane/Instrumentation.h>

#include <cassert>

namespace Methane::Graphics
{

CD3DX12_VIEWPORT TypeConverterDX::ViewportToD3D(const Viewport& viewport) noexcept
{
    ITT_FUNCTION_TASK();
    return CD3DX12_VIEWPORT(static_cast<float>(viewport.origin.GetX()), static_cast<float>(viewport.origin.GetY()),
                            static_cast<float>(viewport.size.width), static_cast<float>(viewport.size.height),
                            static_cast<float>(viewport.origin.GetZ()), static_cast<float>(viewport.origin.GetZ() + viewport.size.depth));
}

CD3DX12_RECT TypeConverterDX::ScissorRectToD3D(const ScissorRect& scissor_rect) noexcept
{
    ITT_FUNCTION_TASK();
    return CD3DX12_RECT(static_cast<LONG>(scissor_rect.origin.GetX()), static_cast<LONG>(scissor_rect.origin.GetY()),
                        static_cast<LONG>(scissor_rect.origin.GetX() + scissor_rect.size.width),
                        static_cast<LONG>(scissor_rect.origin.GetY() + scissor_rect.size.height));
}

std::vector<CD3DX12_VIEWPORT> TypeConverterDX::ViewportsToD3D(const Viewports& viewports) noexcept
{
    ITT_FUNCTION_TASK();

    std::vector<CD3DX12_VIEWPORT> d3d_viewports;
    for (const Viewport& viewport : viewports)
    {
        d3d_viewports.push_back(ViewportToD3D(viewport));
    }
    return d3d_viewports;
}

std::vector<CD3DX12_RECT> TypeConverterDX::ScissorRectsToD3D(const ScissorRects& scissor_rects) noexcept
{
    ITT_FUNCTION_TASK();

    std::vector<CD3DX12_RECT> d3d_scissor_rects;
    for (const ScissorRect& scissor_rect : scissor_rects)
    {
        d3d_scissor_rects.push_back(ScissorRectToD3D(scissor_rect));
    }
    return d3d_scissor_rects;
}

DXGI_FORMAT TypeConverterDX::DataFormatToDXGI(const PixelFormat& data_format) noexcept
{
    ITT_FUNCTION_TASK();

    switch (data_format)
    {
    case PixelFormat::Unknown:       return DXGI_FORMAT_UNKNOWN;
    case PixelFormat::RGBA8:         return DXGI_FORMAT_R8G8B8A8_TYPELESS;
    case PixelFormat::RGBA8Unorm:    return DXGI_FORMAT_R8G8B8A8_UNORM;
    case PixelFormat::BGRA8Unorm:    return DXGI_FORMAT_B8G8R8A8_UNORM;
    case PixelFormat::Depth32Float:  return DXGI_FORMAT_D32_FLOAT;
    case PixelFormat::R32Float:      return DXGI_FORMAT_R32_FLOAT;
    case PixelFormat::R32Uint:       return DXGI_FORMAT_R32_UINT;
    case PixelFormat::R32Sint:       return DXGI_FORMAT_R32_SINT;
    case PixelFormat::R16Uint:       return DXGI_FORMAT_R16_UINT;
    case PixelFormat::R16Sint:       return DXGI_FORMAT_R16_SINT;
    default:                         assert(0);
    }
    return DXGI_FORMAT_UNKNOWN;
}

DXGI_FORMAT TypeConverterDX::DataFormatToDXGI(const PixelFormat& data_format, ResourceFormatType format_type) noexcept
{
    ITT_FUNCTION_TASK();

    switch (data_format)
    {
    case PixelFormat::Depth32Float:
    {
        switch (format_type)
        {
        case ResourceFormatType::ResourceBase:  return DXGI_FORMAT_R32_TYPELESS;
        case ResourceFormatType::ViewRead:  return DXGI_FORMAT_R32_FLOAT;
        case ResourceFormatType::ViewWrite: return DXGI_FORMAT_D32_FLOAT;
        }
    } break;

    default: assert(0);
    }
    return DataFormatToDXGI(data_format);
}

D3D12_COMPARISON_FUNC TypeConverterDX::CompareFunctionToDX(Compare compare_func) noexcept
{
    ITT_FUNCTION_TASK();

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
    default:                            assert(0);
    }
    return D3D12_COMPARISON_FUNC_NEVER;
}

DXGI_FORMAT TypeConverterDX::ParameterDescToDxgiFormatAndSize(const D3D12_SIGNATURE_PARAMETER_DESC& param_desc, uint32_t& out_element_byte_size) noexcept
{
    ITT_FUNCTION_TASK();

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

    assert(0);
    out_element_byte_size = 0;
    return DXGI_FORMAT_UNKNOWN;
}

} // namespace Methane::Graphics
