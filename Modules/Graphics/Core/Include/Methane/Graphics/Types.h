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

FILE: Methane/Graphics/Types.h
Methane primitive graphics types.

******************************************************************************/

#pragma once

#include <Methane/Data/Types.h>
#include <Methane/Graphics/MathTypes.h>

#include <cstdint>

namespace Methane::Graphics
{

template<typename T>
using Point2T = Data::Point2T<T>;

using Point2i = Point2T<int32_t>;
using Point2u = Point2T<uint32_t>;
using Point2f = Point2T<float>;
using Point2d = Point2T<double>;

template<typename T, typename D>
using Rect = Data::Rect<T, D>;

using FrameRect    = Data::FrameRect;
using FrameSize    = Data::FrameSize;
using ScissorRect  = Rect<uint32_t, uint32_t>;
using ScissorRects = std::vector<ScissorRect>;

ScissorRect GetFrameScissorRect(const FrameRect& frame_rect);
ScissorRect GetFrameScissorRect(const FrameSize& frame_size);

template<typename T>
class Point3T : public cml::vector<T, cml::fixed<3>>
{
public:
    Point3T() = default;
    Point3T(T x, T y, T z) : cml::vector<T, cml::fixed<3>>(x, y, z) { }
    Point3T(const cml::vector<T, cml::fixed<3>>& v) : cml::vector<T, cml::fixed<3>>(v) { }
    
    T GetX() const noexcept { return (*this)[0]; }
    T GetY() const noexcept { return (*this)[1]; }
    T GetZ() const noexcept { return (*this)[2]; }
    
    void SetX(T x) noexcept { (*this)[0] = x; }
    void SetY(T y) noexcept { (*this)[1] = y; }
    void SetZ(T z) noexcept { (*this)[2] = z; }

    operator std::string() const
    { return "Pt(" + std::to_string(GetX()) + ", " + std::to_string(GetY()) + ", " + std::to_string(GetZ()) + ")"; }
};

using Point3i = Point3T<int32_t>;
using Point3u = Point3T<uint32_t>;
using Point3f = Point3T<float>;
using Point3d = Point3T<double>;

template<typename T, typename D>
struct Volume
{
    struct Size : Rect<T, D>::Size
    {
        D depth = 1;
        
        Size() = default;
        Size(const typename Rect<T, D>::Size& rect_size, D d = 1) : Rect<T, D>::Size(rect_size), depth(d) { }
        Size(D w, D h, D d = 1) : Rect<T, D>::Size(w, h), depth(d) { }

        bool operator==(const Size& other) const noexcept
        { return Rect<T, D>::Size::operator==(other) && depth == other.depth; }

        bool operator!=(const Size& other) const noexcept
        { return Rect<T, D>::Size::operator!=(other) || depth != other.depth; }

        D GetPixelsCount() const noexcept { return depth * Rect<T, D>::Size::GetPixelsCount(); }
        D GetLongestSide() const noexcept { return std::max(depth, Rect<T, D>::Size::GetLongestSide()); }

        operator std::string() const
        {
            return "Sz(" + std::to_string(Rect<T, D>::Size::width) +
                   " x " + std::to_string(Rect<T, D>::Size::height) +
                   " x " + std::to_string(depth) + ")";
        }
    };

    bool operator==(const Volume& other) const noexcept
    { return std::tie(origin, size) == std::tie(other.origin, other.size); }

    bool operator!=(const Volume& other) const noexcept
    { return std::tie(origin, size) != std::tie(other.origin, other.size); }

    operator std::string() const
    { return std::string("Vm[") + origin + " + " + size + "]"; }

    using Point = Point3T<T>;

    Point origin;
    Size  size;
};

using Dimensions = Volume<int32_t, uint32_t>::Size;

using Viewport  = Volume<double, double>;
using Viewports = std::vector<Viewport>;

Viewport GetFrameViewport(const FrameSize& frame_size);
Viewport GetFrameViewport(const FrameRect& frame_rect);
    
class Color3f : public Vector3f
{
public:
    using Vector3f::Vector3f;
    using Vector3f::operator=;
    
    Color3f() = default;
    Color3f(float r, float g, float b) : Vector3f(r, g, b) { }
    
    float GetR() const noexcept { return (*this)[0]; }
    float GetG() const noexcept { return (*this)[1]; }
    float GetB() const noexcept { return (*this)[2]; }
    
    void SetR(float r) noexcept { (*this)[0] = r; }
    void SetG(float g) noexcept { (*this)[1] = g; }
    void SetB(float b) noexcept { (*this)[2] = b; }

    std::string ToString() const
    {
        return "C(R:" + std::to_string(GetR()) +
               ", G:" + std::to_string(GetG()) +
               ", B:" + std::to_string(GetB()) + ")";
    }
};

class Color4f : public Vector4f
{
public:
    using Vector4f::Vector4f;
    using Vector4f::operator=;

    Color4f() : Vector4f(0.f, 0.f, 0.f, 0.f) { }
    Color4f(float r, float g, float b, float a) : Vector4f(r, g, b, a) { }
    
    float GetR() const noexcept { return (*this)[0]; }
    float GetG() const noexcept { return (*this)[1]; }
    float GetB() const noexcept { return (*this)[2]; }
    float GetA() const noexcept { return (*this)[3]; }
    
    void SetR(float r) noexcept { (*this)[0] = r; }
    void SetG(float g) noexcept { (*this)[1] = g; }
    void SetB(float b) noexcept { (*this)[2] = b; }
    void SetA(float a) noexcept { (*this)[3] = a; }

    std::string ToString() const
    {
        return "C(R:" + std::to_string(GetR()) +
               ", G:" + std::to_string(GetG()) +
               ", B:" + std::to_string(GetB()) +
               ", A:" + std::to_string(GetA()) + ")";
    }
};

using Depth = float;
using Stencil = uint8_t;
using DepthStencil = std::pair<Depth, Stencil>;

enum class PixelFormat
{
    Unknown,
    RGBA8,
    RGBA8Unorm,
    BGRA8Unorm,
    R32Float,
    R32Uint,
    R32Sint,
    R16Uint,
    R16Sint,
    Depth32Float
};

using PixelFormats = std::vector<PixelFormat>;

uint32_t GetPixelSize(PixelFormat data_format) noexcept;

template<typename TIndex>
PixelFormat GetIndexFormat(TIndex) noexcept          { return PixelFormat::Unknown; }

template<>
inline PixelFormat GetIndexFormat(uint32_t) noexcept { return PixelFormat::R32Uint; }

template<>
inline PixelFormat GetIndexFormat(int32_t) noexcept  { return PixelFormat::R32Sint; }

template<>
inline PixelFormat GetIndexFormat(uint16_t) noexcept { return PixelFormat::R16Uint; }

template<>
inline PixelFormat GetIndexFormat(int16_t) noexcept  { return PixelFormat::R16Sint; }

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
