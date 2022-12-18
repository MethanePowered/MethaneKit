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

FILE: Methane/Graphics/Types.h
Methane primitive graphics types.

******************************************************************************/

#pragma once

#include <Methane/Data/Types.h>
#include <Methane/Data/Constants.hpp>

#include <string>
#include <cstdint>

// NOSONAR
#define META_UNIFORM_ALIGN alignas(Methane::Graphics::g_uniform_alignment)

namespace Methane::Graphics
{

constexpr size_t g_uniform_alignment = 256;

using Depth = float;
using Stencil = uint8_t;
using DepthStencilValues = std::pair<Depth, Stencil>;

using Timestamp = Data::Timestamp;
using TimeDelta = Data::TimeDelta;
using Frequency = Data::Frequency;

template<typename T, size_t size>
using HlslVector = Data::HlslVector<T, size>;

template<typename T, size_t size>
using RawVector = Data::RawVector<T, size>;
using RawVector2F = Data::RawVector2F;
using RawVector3F = Data::RawVector3F;
using RawVector4F = Data::RawVector4F;

template<typename T>
using Constants   = Data::Constants<T>;
using ConstFloat  = Data::ConstFloat;
using ConstDouble = Data::ConstDouble;

enum class PixelFormat : uint32_t
{
    Unknown = 0U,
    RGBA8,
    RGBA8Unorm,
    RGBA8Unorm_sRGB,
    BGRA8Unorm,
    BGRA8Unorm_sRGB,
    R32Float,
    R32Uint,
    R32Sint,
    R16Float,
    R16Uint,
    R16Sint,
    R16Unorm,
    R16Snorm,
    R8Uint,
    R8Sint,
    R8Unorm,
    R8Snorm,
    A8Unorm,
    Depth32Float
};

using PixelFormats = std::vector<PixelFormat>;

struct AttachmentFormats
{
    PixelFormats colors;
    PixelFormat  depth    = PixelFormat::Unknown;
    PixelFormat  stencil  = PixelFormat::Unknown;
};

[[nodiscard]] Data::Size GetPixelSize(PixelFormat pixel_format);
[[nodiscard]] bool IsSrgbColorSpace(PixelFormat pixel_format) noexcept;
[[nodiscard]] bool IsDepthFormat(PixelFormat pixel_format) noexcept;

enum class Compare : uint32_t
{
    Never = 0,
    Always,
    Less,
    Greater,
    LessEqual,
    GreaterEqual,
    Equal,
    NotEqual,
};

} // namespace Methane::Graphics
