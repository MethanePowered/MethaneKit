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

FILE: Methane/Graphics/Metal/Types.mm
Methane graphics types converters to Metal native types.

******************************************************************************/

#include <Methane/Graphics/Metal/Types.hh>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics::Metal
{

constexpr uint32_t g_component_64bit_byte_size = 8;
constexpr uint32_t g_component_32bit_byte_size = 4;
constexpr uint32_t g_component_16bit_byte_size = 2;

MTLIndexType TypeConverter::DataFormatToMetalIndexType(PixelFormat data_format)
{
    META_FUNCTION_TASK();

    switch (data_format)
    {
        using enum PixelFormat;
        case R32Uint: return MTLIndexTypeUInt32;
        case R16Uint: return MTLIndexTypeUInt16;
        default:      META_UNEXPECTED_RETURN(data_format, MTLIndexTypeUInt32);
    }
}

MTLPixelFormat TypeConverter::DataFormatToMetalPixelType(PixelFormat data_format)
{
    META_FUNCTION_TASK();

    switch (data_format)
    {
    using enum PixelFormat;
    case Unknown:          return MTLPixelFormatInvalid;
    case RGBA8:            return MTLPixelFormatRGBA8Uint;
    case RGBA8Unorm:       return MTLPixelFormatRGBA8Unorm;
    case RGBA8Unorm_sRGB:  return MTLPixelFormatRGBA8Unorm_sRGB;
    case BGRA8Unorm:       return MTLPixelFormatBGRA8Unorm;
    case BGRA8Unorm_sRGB:  return MTLPixelFormatBGRA8Unorm_sRGB;
    case R32Float:         return MTLPixelFormatR32Float;
    case R32Uint:          return MTLPixelFormatR32Uint;
    case R32Sint:          return MTLPixelFormatR32Sint;
    case R16Float:         return MTLPixelFormatR16Float;
    case R16Uint:          return MTLPixelFormatR16Uint;
    case R16Sint:          return MTLPixelFormatR16Sint;
    case R16Unorm:         return MTLPixelFormatR16Unorm;
    case R16Snorm:         return MTLPixelFormatR16Snorm;
    case R8Uint:           return MTLPixelFormatR8Uint;
    case R8Sint:           return MTLPixelFormatR8Sint;
    case R8Unorm:          return MTLPixelFormatR8Unorm;
    case R8Snorm:          return MTLPixelFormatR8Snorm;
    case A8Unorm:          return MTLPixelFormatA8Unorm;
    case Depth32Float:     return MTLPixelFormatDepth32Float;
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
    default: META_UNEXPECTED_RETURN(data_format, MTLPixelFormatInvalid);
    }
}

MTLVertexFormat TypeConverter::MetalDataTypeToVertexFormat(MTLDataType data_type, bool normalized)
{
    META_FUNCTION_TASK();

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

        default:                    META_UNEXPECTED_RETURN(data_type, MTLVertexFormatInvalid);
    }
}

uint32_t TypeConverter::ByteSizeOfVertexFormat(MTLVertexFormat vertex_format)
{
    META_FUNCTION_TASK();
    switch(vertex_format)
    {
        case MTLVertexFormatFloat:
        case MTLVertexFormatInt:
        case MTLVertexFormatUInt:
            return 1 * g_component_32bit_byte_size;

        case MTLVertexFormatFloat2:
        case MTLVertexFormatInt2:
        case MTLVertexFormatUInt2:
            return 2 * g_component_32bit_byte_size;

        case MTLVertexFormatFloat3:
        case MTLVertexFormatInt3:
        case MTLVertexFormatUInt3:
            return 3 * g_component_32bit_byte_size;

        case MTLVertexFormatFloat4:
        case MTLVertexFormatInt4:
        case MTLVertexFormatUInt4:
            return 4 * g_component_32bit_byte_size;

        case MTLVertexFormatHalf:
        case MTLVertexFormatShort:
        case MTLVertexFormatShortNormalized:
        case MTLVertexFormatUShort:
        case MTLVertexFormatUShortNormalized:
            return 1 * g_component_16bit_byte_size;

        case MTLVertexFormatHalf2:
        case MTLVertexFormatShort2:
        case MTLVertexFormatShort2Normalized:
        case MTLVertexFormatUShort2:
        case MTLVertexFormatUShort2Normalized:
            return 2 * g_component_16bit_byte_size;

        case MTLVertexFormatHalf3:
        case MTLVertexFormatShort3:
        case MTLVertexFormatShort3Normalized:
        case MTLVertexFormatUShort3:
        case MTLVertexFormatUShort3Normalized:
            return 3 * g_component_16bit_byte_size;

        case MTLVertexFormatHalf4:
        case MTLVertexFormatShort4:
        case MTLVertexFormatShort4Normalized:
        case MTLVertexFormatUShort4:
        case MTLVertexFormatUShort4Normalized:
            return 4 * g_component_16bit_byte_size;
            
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
            META_UNEXPECTED_RETURN(vertex_format, 0);
    }
}

uint32_t TypeConverter::ByteSizeOfDataType(MTLDataType data_type)
{
    META_FUNCTION_TASK();
    switch(data_type)
    {
        case MTLDataTypeLong:
        case MTLDataTypeULong:
            return 1U * g_component_64bit_byte_size;

        case MTLDataTypeLong2:
        case MTLDataTypeULong2:
            return 2U * g_component_64bit_byte_size;

        case MTLDataTypeLong3:
        case MTLDataTypeULong3:
            return 3U * g_component_64bit_byte_size;

        case MTLDataTypeLong4:
        case MTLDataTypeULong4:
            return 4U * g_component_64bit_byte_size;

        case MTLDataTypeFloat:
        case MTLDataTypeInt:
        case MTLDataTypeUInt:
        case MTLDataTypeRGB10A2Unorm:
        case MTLDataTypeRG11B10Float:
        case MTLDataTypeRGB9E5Float:
            return 1U * g_component_32bit_byte_size;

        case MTLDataTypeFloat2:
        case MTLDataTypeInt2:
        case MTLDataTypeUInt2:
            return 2U * g_component_32bit_byte_size;

        case MTLDataTypeFloat3:
        case MTLDataTypeInt3:
        case MTLDataTypeUInt3:
            return 3U * g_component_32bit_byte_size;

        case MTLDataTypeFloat4:
        case MTLDataTypeInt4:
        case MTLDataTypeUInt4:
        case MTLDataTypeFloat2x2:
            return 4U * g_component_32bit_byte_size;

        case MTLDataTypeFloat2x3:
        case MTLDataTypeFloat3x2:
            return 6U * g_component_32bit_byte_size;

        case MTLDataTypeFloat2x4:
        case MTLDataTypeFloat4x2:
            return 8U * g_component_32bit_byte_size;

        case MTLDataTypeFloat3x3:
            return 9U * g_component_32bit_byte_size;

        case MTLDataTypeFloat3x4:
        case MTLDataTypeFloat4x3:
            return 12U * g_component_32bit_byte_size;

        case MTLDataTypeFloat4x4:
            return 16U * g_component_32bit_byte_size;

        case MTLDataTypeHalf:
        case MTLDataTypeBFloat:
        case MTLDataTypeShort:
        case MTLDataTypeUShort:
        case MTLDataTypeR16Unorm:
        case MTLDataTypeR16Snorm:
            return 1U * g_component_16bit_byte_size;

        case MTLDataTypeHalf2:
        case MTLDataTypeBFloat2:
        case MTLDataTypeShort2:
        case MTLDataTypeUShort2:
        case MTLDataTypeRG16Unorm:
        case MTLDataTypeRG16Snorm:
            return 2U * g_component_16bit_byte_size;

        case MTLDataTypeHalf3:
        case MTLDataTypeBFloat3:
        case MTLDataTypeShort3:
        case MTLDataTypeUShort3:
        case MTLDataTypeRGBA8Unorm:
        case MTLDataTypeRGBA8Unorm_sRGB:
            return 3U * g_component_16bit_byte_size;

        case MTLDataTypeHalf4:
        case MTLDataTypeBFloat4:
        case MTLDataTypeShort4:
        case MTLDataTypeUShort4:
        case MTLDataTypeHalf2x2:
        case MTLDataTypeRGBA16Unorm:
        case MTLDataTypeRGBA16Snorm:
            return 4U * g_component_16bit_byte_size;

        case MTLDataTypeHalf2x3:
        case MTLDataTypeHalf3x2:
            return 6U * g_component_16bit_byte_size;

        case MTLDataTypeHalf2x4:
        case MTLDataTypeHalf4x2:
            return 8U * g_component_16bit_byte_size;

        case MTLDataTypeHalf3x3:
            return 9U * g_component_16bit_byte_size;

        case MTLDataTypeHalf3x4:
        case MTLDataTypeHalf4x3:
            return 12U * g_component_16bit_byte_size;

        case MTLDataTypeHalf4x4:
            return 16U * g_component_16bit_byte_size;

        case MTLDataTypeBool:
        case MTLDataTypeChar:
        case MTLDataTypeUChar:
        case MTLDataTypeR8Unorm:
        case MTLDataTypeR8Snorm:
            return 1U;

        case MTLDataTypeBool2:
        case MTLDataTypeChar2:
        case MTLDataTypeUChar2:
        case MTLDataTypeRG8Unorm:
        case MTLDataTypeRG8Snorm:
            return 2U;

        case MTLDataTypeBool3:
        case MTLDataTypeChar3:
        case MTLDataTypeUChar3:
            return 3U;

        case MTLDataTypeBool4:
        case MTLDataTypeChar4:
        case MTLDataTypeUChar4:
        case MTLDataTypeRGBA8Snorm:
            return 4U;

        case MTLDataTypeNone:
            return 0U;

        default:
            META_UNEXPECTED_RETURN(data_type, 0U);
    }
}

MTLClearColor TypeConverter::ColorToMetalClearColor(const Color4F& color) noexcept
{
    META_FUNCTION_TASK();
    return MTLClearColorMake(color.GetRed(), color.GetGreen(), color.GetBlue(), color.GetAlpha());
}

NativeRect TypeConverter::RectToNS(const FrameRect& rect) noexcept
{
    META_FUNCTION_TASK();
    return CreateNSRect(rect.size, rect.origin);
}

NativeRect TypeConverter::CreateNSRect(const FrameSize& size, const Point2I& origin) noexcept
{
    META_FUNCTION_TASK();
    return MakeNativeRect(origin.GetX(), origin.GetY(), size.GetWidth(), size.GetHeight());
}

FrameRect TypeConverter::RectFromNS(const NativeRect& rect) noexcept
{
    META_FUNCTION_TASK();
    return FrameRect {
        Point2I(rect.origin.x, rect.origin.y),
        FrameSize(rect.size.width, rect.size.height)
    };
}

MTLCompareFunction TypeConverter::CompareFunctionToMetal(Compare compare_func)
{
    META_FUNCTION_TASK();
    switch(compare_func)
    {
        using enum Compare;
        case Never:        return MTLCompareFunctionNever;
        case Always:       return MTLCompareFunctionAlways;
        case Less:         return MTLCompareFunctionLess;
        case LessEqual:    return MTLCompareFunctionLessEqual;
        case Greater:      return MTLCompareFunctionGreater;
        case GreaterEqual: return MTLCompareFunctionGreaterEqual;
        case Equal:        return MTLCompareFunctionEqual;
        case NotEqual:     return MTLCompareFunctionNotEqual;
        default:           META_UNEXPECTED_RETURN(compare_func, MTLCompareFunctionNever);
    }
}

} // namespace Methane::Graphics::Metal
