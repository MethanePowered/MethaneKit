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

FILE: Methane/Graphics/Vulkan/TypesVK.mm
Methane graphics types converters to Vulkan native types.

******************************************************************************/

#include "TypesVK.h"

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics
{

vk::Format TypeConverterVK::PixelFormatToVulkan(PixelFormat pixel_format)
{
    META_FUNCTION_TASK();
    switch (pixel_format)
    {
    case PixelFormat::Unknown:          return vk::Format::eUndefined;
    case PixelFormat::RGBA8:            return vk::Format::eR8G8B8A8Uint;
    case PixelFormat::RGBA8Unorm:       return vk::Format::eR8G8B8A8Unorm;
    case PixelFormat::RGBA8Unorm_sRGB:  return vk::Format::eR8G8B8A8Srgb;
    case PixelFormat::BGRA8Unorm:       return vk::Format::eB8G8R8A8Unorm;
    case PixelFormat::BGRA8Unorm_sRGB:  return vk::Format::eB8G8R8A8Srgb;
    case PixelFormat::Depth32Float:     return vk::Format::eD32Sfloat;
    case PixelFormat::R32Float:         return vk::Format::eR32Sfloat;
    case PixelFormat::R32Uint:          return vk::Format::eR32Uint;
    case PixelFormat::R32Sint:          return vk::Format::eR32Sint;
    case PixelFormat::R16Float:         return vk::Format::eR16Sfloat;
    case PixelFormat::R16Uint:          return vk::Format::eR16Uint;
    case PixelFormat::R16Sint:          return vk::Format::eR16Sint;
    case PixelFormat::R16Unorm:         return vk::Format::eR16Unorm;
    case PixelFormat::R16Snorm:         return vk::Format::eR16Snorm;
    case PixelFormat::R8Uint:           return vk::Format::eR8Uint;
    case PixelFormat::R8Sint:           return vk::Format::eR8Sint;
    case PixelFormat::R8Unorm:          return vk::Format::eR8Unorm;
    case PixelFormat::R8Snorm:          return vk::Format::eR8Snorm;
    case PixelFormat::A8Unorm:          return vk::Format::eR8Unorm; // TODO: Channels swizzle?
    default:                            META_UNEXPECTED_ARG_RETURN(pixel_format, vk::Format::eUndefined);
    }
}

} // namespace Methane::Graphics
