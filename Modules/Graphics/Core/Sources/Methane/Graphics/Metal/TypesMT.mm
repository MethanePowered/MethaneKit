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

FILE: Methane/Graphics/Metal/TypesMT.mm
Methane graphics types converters to Metal native types.

******************************************************************************/

#include "TypesMT.hh"

#include <Methane/Instrumentation.h>

#include <cassert>

namespace Methane::Graphics
{

MTLIndexType TypeConverterMT::DataFormatToMetalIndexType(PixelFormat data_format) noexcept
{
    ITT_FUNCTION_TASK();

    switch (data_format)
    {
        case PixelFormat::R32Uint:       return MTLIndexTypeUInt32;
        case PixelFormat::R16Uint:       return MTLIndexTypeUInt16;
        default:                        assert(0);
    }
    return MTLIndexTypeUInt32;
}

MTLPixelFormat TypeConverterMT::DataFormatToMetalPixelType(PixelFormat data_format) noexcept
{
    ITT_FUNCTION_TASK();

    switch (data_format)
    {
        case PixelFormat::Unknown:       return MTLPixelFormatInvalid;
        case PixelFormat::RGBA8:         return MTLPixelFormatRGBA8Uint;
        case PixelFormat::RGBA8Unorm:    return MTLPixelFormatRGBA8Unorm;
        case PixelFormat::BGRA8Unorm:    return MTLPixelFormatBGRA8Unorm;
        case PixelFormat::R32Float:      return MTLPixelFormatR32Float;
        case PixelFormat::R32Uint:       return MTLPixelFormatR32Uint;
        case PixelFormat::R32Sint:       return MTLPixelFormatR32Sint;
        case PixelFormat::R16Uint:       return MTLPixelFormatRG16Uint;
        case PixelFormat::R16Sint:       return MTLPixelFormatRG16Sint;
        case PixelFormat::Depth32Float:  return MTLPixelFormatDepth32Float;
        // MTLPixelFormatA8Unorm;
        // MTLPixelFormatR8Unorm;
        // MTLPixelFormatR8Snorm;
        // MTLPixelFormatR8Uint;
        // MTLPixelFormatR8Sint;
        // MTLPixelFormatR16Unorm;
        // MTLPixelFormatR16Snorm;
        // MTLPixelFormatR16Uint;
        // MTLPixelFormatR16Sint;
        // MTLPixelFormatR16Float;
        // MTLPixelFormatRG8Unorm;
        // MTLPixelFormatRG8Snorm;
        // MTLPixelFormatRG8Uint;
        // MTLPixelFormatRG8Sint;
        // MTLPixelFormatRG16Unorm;
        // MTLPixelFormatRG16Snorm;
        // MTLPixelFormatRG16Float;
        // MTLPixelFormatRGBA8Unorm_sRGB;
        // MTLPixelFormatRGBA8Snorm;
        // MTLPixelFormatRGBA8Sint;
        // MTLPixelFormatBGRA8Unorm_sRGB;
        // MTLPixelFormatRGB10A2Unorm;
        // MTLPixelFormatRGB10A2Uint;
        // MTLPixelFormatRG11B10Float;
        // MTLPixelFormatRGB9E5Float;
        // MTLPixelFormatBGR10A2Unorm;
        // MTLPixelFormatRG32Uint;
        // MTLPixelFormatRG32Sint;
        // MTLPixelFormatRG32Float;
        // MTLPixelFormatRGBA16Unorm;
        // MTLPixelFormatRGBA16Snorm;
        // MTLPixelFormatRGBA16Uint;
        // MTLPixelFormatRGBA16Sint;
        // MTLPixelFormatRGBA16Float;
        // MTLPixelFormatRGBA32Uint;
        // MTLPixelFormatRGBA32Sint;
        // MTLPixelFormatRGBA32Float;
        // MTLPixelFormatBC1_RGBA;
        // MTLPixelFormatBC1_RGBA_sRGB;
        // MTLPixelFormatBC2_RGBA;
        // MTLPixelFormatBC2_RGBA_sRGB;
        // MTLPixelFormatBC3_RGBA;
        // MTLPixelFormatBC3_RGBA_sRGB;
        // MTLPixelFormatBC4_RUnorm;
        // MTLPixelFormatBC4_RSnorm;
        // MTLPixelFormatBC5_RGUnorm;
        // MTLPixelFormatBC5_RGSnorm;
        // MTLPixelFormatBC6H_RGBFloat;
        // MTLPixelFormatBC6H_RGBUfloat;
        // MTLPixelFormatBC7_RGBAUnorm;
        // MTLPixelFormatBC7_RGBAUnorm_sRGB;
        // MTLPixelFormatGBGR422;
        // MTLPixelFormatBGRG422;
        // MTLPixelFormatDepth16Unorm;
        // MTLPixelFormatStencil8;
        // MTLPixelFormatDepth24Unorm_Stencil8;
        // MTLPixelFormatDepth32Float_Stencil8;
        // MTLPixelFormatX32_Stencil8;
        // MTLPixelFormatX24_Stencil8;
    }
    return MTLPixelFormatInvalid;
}

MTLVertexFormat TypeConverterMT::MetalDataTypeToVertexFormat(MTLDataType data_type, bool normalized) noexcept
{
    ITT_FUNCTION_TASK();

    switch(data_type)
    {
        case MTLDataTypeFloat:      return MTLVertexFormatFloat;
        case MTLDataTypeFloat2:     return MTLVertexFormatFloat2;
        case MTLDataTypeFloat3:     return MTLVertexFormatFloat3;
        case MTLDataTypeFloat4:     return MTLVertexFormatFloat4;
            
        case MTLDataTypeHalf:       return MTLVertexFormatHalf;
        case MTLDataTypeHalf2:      return MTLVertexFormatHalf2;
        case MTLDataTypeHalf3:      return MTLVertexFormatHalf3;
        case MTLDataTypeHalf4:      return MTLVertexFormatHalf4;
            
        case MTLDataTypeInt:        return MTLVertexFormatInt;
        case MTLDataTypeInt2:       return MTLVertexFormatInt2;
        case MTLDataTypeInt3:       return MTLVertexFormatInt3;
        case MTLDataTypeInt4:       return MTLVertexFormatInt4;
            
        case MTLDataTypeUInt:       return MTLVertexFormatUInt;
        case MTLDataTypeUInt2:      return MTLVertexFormatUInt2;
        case MTLDataTypeUInt3:      return MTLVertexFormatUInt3;
        case MTLDataTypeUInt4:      return MTLVertexFormatUInt4;
            
        case MTLDataTypeShort:      return normalized ? MTLVertexFormatShort  : MTLVertexFormatShortNormalized;
        case MTLDataTypeShort2:     return normalized ? MTLVertexFormatShort2 : MTLVertexFormatShort2Normalized;
        case MTLDataTypeShort3:     return normalized ? MTLVertexFormatShort3 : MTLVertexFormatShort3Normalized;
        case MTLDataTypeShort4:     return normalized ? MTLVertexFormatShort4 : MTLVertexFormatShort4Normalized;
            
        case MTLDataTypeUShort:     return normalized ? MTLVertexFormatUShort  : MTLVertexFormatUShortNormalized;
        case MTLDataTypeUShort2:    return normalized ? MTLVertexFormatUShort2 : MTLVertexFormatUShort2Normalized;
        case MTLDataTypeUShort3:    return normalized ? MTLVertexFormatUShort3 : MTLVertexFormatUShort3Normalized;
        case MTLDataTypeUShort4:    return normalized ? MTLVertexFormatUShort4 : MTLVertexFormatUShort4Normalized;
            
        case MTLDataTypeChar:       return normalized ? MTLVertexFormatChar  : MTLVertexFormatCharNormalized;
        case MTLDataTypeChar2:      return normalized ? MTLVertexFormatChar2 : MTLVertexFormatChar2Normalized;
        case MTLDataTypeChar3:      return normalized ? MTLVertexFormatChar3 : MTLVertexFormatChar3Normalized;
        case MTLDataTypeChar4:      return normalized ? MTLVertexFormatChar4 : MTLVertexFormatChar4Normalized;
            
        case MTLDataTypeUChar:      return normalized ? MTLVertexFormatUChar  : MTLVertexFormatUCharNormalized;
        case MTLDataTypeUChar2:     return normalized ? MTLVertexFormatUChar2 : MTLVertexFormatUChar2Normalized;
        case MTLDataTypeUChar3:     return normalized ? MTLVertexFormatUChar3 : MTLVertexFormatUChar3Normalized;
        case MTLDataTypeUChar4:     return normalized ? MTLVertexFormatUChar4 : MTLVertexFormatUChar4Normalized;

        default:                    assert(0);
    }
    return MTLVertexFormatInvalid;
}

uint32_t TypeConverterMT::ByteSizeOfVertexFormat(MTLVertexFormat vertex_format) noexcept
{
    ITT_FUNCTION_TASK();

    const uint32_t component_32bit_byte_size = 4;
    const uint32_t component_16bit_byte_size = 2;
    
    switch(vertex_format)
    {
        case MTLVertexFormatFloat:
        case MTLVertexFormatInt:
        case MTLVertexFormatUInt:
            return 1 * component_32bit_byte_size;

        case MTLVertexFormatFloat2:
        case MTLVertexFormatInt2:
        case MTLVertexFormatUInt2:
            return 2 * component_32bit_byte_size;

        case MTLVertexFormatFloat3:
        case MTLVertexFormatInt3:
        case MTLVertexFormatUInt3:
            return 3 * component_32bit_byte_size;

        case MTLVertexFormatFloat4:
        case MTLVertexFormatInt4:
        case MTLVertexFormatUInt4:
            return 4 * component_32bit_byte_size;

        case MTLVertexFormatHalf:
        case MTLVertexFormatShort:
        case MTLVertexFormatShortNormalized:
        case MTLVertexFormatUShort:
        case MTLVertexFormatUShortNormalized:
            return 1 * component_16bit_byte_size;

        case MTLVertexFormatHalf2:
        case MTLVertexFormatShort2:
        case MTLVertexFormatShort2Normalized:
        case MTLVertexFormatUShort2:
        case MTLVertexFormatUShort2Normalized:
            return 2 * component_16bit_byte_size;

        case MTLVertexFormatHalf3:
        case MTLVertexFormatShort3:
        case MTLVertexFormatShort3Normalized:
        case MTLVertexFormatUShort3:
        case MTLVertexFormatUShort3Normalized:
            return 3 * component_16bit_byte_size;

        case MTLVertexFormatHalf4:
        case MTLVertexFormatShort4:
        case MTLVertexFormatShort4Normalized:
        case MTLVertexFormatUShort4:
        case MTLVertexFormatUShort4Normalized:
            return 4 * component_16bit_byte_size;
            
        case MTLVertexFormatChar:
        case MTLVertexFormatCharNormalized:
        case MTLVertexFormatUChar:
        case MTLVertexFormatUCharNormalized:
            return 1;

        case MTLVertexFormatChar2:
        case MTLVertexFormatChar2Normalized:
        case MTLVertexFormatUChar2:
        case MTLVertexFormatUChar2Normalized:
            return 2;

        case MTLVertexFormatChar3:
        case MTLVertexFormatChar3Normalized:
        case MTLVertexFormatUChar3:
        case MTLVertexFormatUChar3Normalized:
            return 3;

        case MTLVertexFormatChar4:
        case MTLVertexFormatChar4Normalized:
        case MTLVertexFormatUChar4:
        case MTLVertexFormatUChar4Normalized:
            return 4;

        default:
            assert(0);
    }
    return 0;
}

MTLClearColor TypeConverterMT::ColorToMetalClearColor(const Color4f& color) noexcept
{
    ITT_FUNCTION_TASK();
    return MTLClearColorMake(color.GetR(), color.GetG(), color.GetB(), color.GetA());
}

NSRect TypeConverterMT::RectToNS(const FrameRect& rect) noexcept
{
    ITT_FUNCTION_TASK();
    return CreateNSRect(rect.size, rect.origin);
}

NSRect TypeConverterMT::CreateNSRect(const FrameSize& size, const Point2i& origin) noexcept
{
    ITT_FUNCTION_TASK();
    return NSMakeRect(origin.GetX(), origin.GetY(), size.width, size.height);
}

FrameRect TypeConverterMT::RectFromNS(const NSRect& rect) noexcept
{
    ITT_FUNCTION_TASK();
    return FrameRect {
        Point2i(static_cast<uint32_t>(rect.origin.x), static_cast<uint32_t>(rect.origin.y)),
        FrameSize(static_cast<uint32_t>(rect.size.width), static_cast<uint32_t>(rect.size.height))
    };
}

MTLCompareFunction TypeConverterMT::CompareFunctionToMetal(Compare compare_func) noexcept
{
    ITT_FUNCTION_TASK();
    switch(compare_func)
    {
        case Compare::Never:        return MTLCompareFunctionNever;
        case Compare::Always:       return MTLCompareFunctionAlways;
        case Compare::Less:         return MTLCompareFunctionLess;
        case Compare::LessEqual:    return MTLCompareFunctionLessEqual;
        case Compare::Greater:      return MTLCompareFunctionGreater;
        case Compare::GreaterEqual: return MTLCompareFunctionGreaterEqual;
        case Compare::Equal:        return MTLCompareFunctionEqual;
        case Compare::NotEqual:     return MTLCompareFunctionNotEqual;
    }
    return MTLCompareFunctionNever;
}

} // namespace Methane::Graphics
