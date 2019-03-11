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

FILE: Methane/Graphics/Types.cpp
Methane primitive graphics types.

******************************************************************************/

#include <Methane/Graphics/Types.h>

#include "Instrumentation.h"

#include <cassert>

namespace Methane
{
namespace Graphics
{

ScissorRect GetFrameScissorRect(const FrameSize& frame_size)
{
    ITT_FUNCTION_TASK();
    return ScissorRect {
        ScissorRect::Point(0, 0),
        ScissorRect::Size(frame_size.width, frame_size.height)
    };
}

Viewport GetFrameViewport(const FrameSize& frame_size)
{
    ITT_FUNCTION_TASK();
    return Viewport {
        Viewport::Point(0.0, 0.0, 0.0),
        Viewport::Size(static_cast<double>(frame_size.width), static_cast<double>(frame_size.height), 1.0)
    };
}

uint32_t GetPixelSize(PixelFormat data_format) noexcept
{
    ITT_FUNCTION_TASK();
    switch(data_format)
    {
        case PixelFormat::RGBA8:
        case PixelFormat::RGBA8Unorm:
        case PixelFormat::BGRA8Unorm:
        case PixelFormat::R32Float:
        case PixelFormat::R32Uint:
        case PixelFormat::R32Sint:
        case PixelFormat::Depth32Float:
            return 4;
        case PixelFormat::R16Uint:
            return 2;
        default:
            assert(0);
    }
    return 0;
}

} // namespace Graphics
} // namespace Methane
