/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/Types.cpp
Methane graphics types converters to Vulkan native types.

******************************************************************************/

#include <Methane/Graphics/Vulkan/Types.h>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics::Vulkan
{

[[nodiscard]]
vk::Format TypeConverter::PixelFormatToVulkan(PixelFormat pixel_format)
{
    META_FUNCTION_TASK();
    switch (pixel_format)
    {
    using enum PixelFormat;
    using enum vk::Format;
    case Unknown:          return eUndefined;
    case RGBA8:            return eR8G8B8A8Uint;
    case RGBA8Unorm:       return eR8G8B8A8Unorm;
    case RGBA8Unorm_sRGB:  return eR8G8B8A8Srgb;
    case BGRA8Unorm:       return eB8G8R8A8Unorm;
    case BGRA8Unorm_sRGB:  return eB8G8R8A8Srgb;
    case Depth32Float:     return eD32Sfloat;
    case R32Float:         return eR32Sfloat;
    case R32Uint:          return eR32Uint;
    case R32Sint:          return eR32Sint;
    case R16Float:         return eR16Sfloat;
    case R16Uint:          return eR16Uint;
    case R16Sint:          return eR16Sint;
    case R16Unorm:         return eR16Unorm;
    case R16Snorm:         return eR16Snorm;
    case R8Uint:           return eR8Uint;
    case R8Sint:           return eR8Sint;
    case R8Unorm:          return eR8Unorm;
    case R8Snorm:          return eR8Snorm;
    case A8Unorm:          return eR8Unorm; // TODO: Channels swizzle?
    default:               META_UNEXPECTED_RETURN(pixel_format, vk::Format::eUndefined);
    }
}

[[nodiscard]]
vk::CompareOp TypeConverter::CompareFunctionToVulkan(Compare compare_func)
{
    META_FUNCTION_TASK();
    switch (compare_func)
    {
    using enum Compare;
    using enum vk::CompareOp;
    case Never:        return eNever;
    case Always:       return eAlways;
    case Less:         return eLess;
    case LessEqual:    return eLessOrEqual;
    case Greater:      return eGreater;
    case GreaterEqual: return eGreaterOrEqual;
    case Equal:        return eEqual;
    case NotEqual:     return eNotEqual;
    default:           META_UNEXPECTED_RETURN(compare_func, vk::CompareOp::eNever);
    }
}

[[nodiscard]]
vk::Extent3D TypeConverter::DimensionsToExtent3D(const Dimensions& dimensions)
{
    META_FUNCTION_TASK();
    return vk::Extent3D(dimensions.GetWidth(), dimensions.GetHeight(), dimensions.GetDepth());
}

[[nodiscard]]
vk::Extent3D TypeConverter::FrameSizeToExtent3D(const FrameSize& frame_size)
{
    META_FUNCTION_TASK();
    return vk::Extent3D(frame_size.GetWidth(), frame_size.GetHeight(), 1U);
}

[[nodiscard]]
vk::SampleCountFlagBits TypeConverter::SampleCountToVulkan(uint32_t sample_count)
{
    META_FUNCTION_TASK();
    switch(sample_count)
    {
    using enum vk::SampleCountFlagBits;
    case 1U:  return e1;
    case 2U:  return e2;
    case 4U:  return e4;
    case 8U:  return e8;
    case 16U: return e16;
    case 32U: return e32;
    case 64U: return e64;
    default:
        META_UNEXPECTED_RETURN_DESCR(sample_count, vk::SampleCountFlagBits::e1,
                                     "Vulkan rasterizer sample count should be a power of 2 from 1 to 64.");
    }
}

} // namespace Methane::Graphics::Vulkan
