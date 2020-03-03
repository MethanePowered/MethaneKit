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

FILE: Methane/Graphics/Metal/TypesMT.hh
Methane graphics types converters to Metal native types.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Types.h>

#import <Metal/Metal.h>

#include <vector>

namespace Methane::Graphics
{

class TypeConverterMT
{
public:
    static MTLIndexType DataFormatToMetalIndexType(PixelFormat data_format) noexcept;
    static MTLPixelFormat DataFormatToMetalPixelType(PixelFormat data_format) noexcept;
    static MTLVertexFormat MetalDataTypeToVertexFormat(MTLDataType data_type, bool normalized = false) noexcept;
    static uint32_t ByteSizeOfVertexFormat(MTLVertexFormat vertex_format) noexcept;
    static MTLClearColor ColorToMetalClearColor(const Color4f& color) noexcept;
    static NSRect RectToNS(const FrameRect& rect) noexcept;
    static NSRect CreateNSRect(const FrameSize& size, const Point2i& origin = Point2i(0, 0)) noexcept;
    static FrameRect RectFromNS(const NSRect& rect) noexcept;
    static MTLCompareFunction CompareFunctionToMetal(Compare compare_func) noexcept;

private:
    TypeConverterMT() = default;
};

} // namespace Methane::Graphics
