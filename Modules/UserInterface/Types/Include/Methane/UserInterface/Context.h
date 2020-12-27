/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/UserInterface/Context.h
Methane user interface context used by all widgets for rendering.

******************************************************************************/

#pragma once

#include "Types.hpp"

#include <Methane/Memory.hpp>

#include <map>

namespace Methane::Graphics
{
struct RenderContext;
struct Object;
}

namespace Methane::UserInterface
{

namespace gfx = Methane::Graphics;

class Context
{
public:
    explicit Context(gfx::RenderContext& render_context) noexcept;

    [[nodiscard]] const gfx::RenderContext& GetRenderContext() const noexcept         { return m_render_context; }
    [[nodiscard]] gfx::RenderContext&       GetRenderContext() noexcept               { return m_render_context; }

    [[nodiscard]] float     GetDotsToPixelsFactor() const noexcept                    { return m_dots_to_pixels_factor; }
    [[nodiscard]] uint32_t  GetFontResolutionDpi() const noexcept                     { return m_font_resolution_dpi; }
    [[nodiscard]] const FrameSize& GetFrameSize() const noexcept;

    [[nodiscard]] UnitSize  GetFrameSizeInPixels() const noexcept                     { return UnitSize(Units::Pixels, GetFrameSize()); }
    [[nodiscard]] UnitSize  GetFrameSizeInDots() const noexcept                       { return UnitSize(Units::Dots, GetFrameSize() / m_dots_to_pixels_factor); }
    [[nodiscard]] UnitSize  GetFrameSizeInUnits(Units units) const noexcept;

    [[nodiscard]] UnitPoint ConvertToPixels(const FloatPoint& point) const noexcept;
    [[nodiscard]] UnitPoint ConvertToDots(const FloatPoint& point) const noexcept;

    [[nodiscard]] UnitSize  ConvertToPixels(const FloatSize& fsize) const noexcept;
    [[nodiscard]] UnitSize  ConvertToDots(const FloatSize& fsize) const noexcept;

    [[nodiscard]] UnitRect  ConvertToPixels(const FloatRect& rect) const noexcept;
    [[nodiscard]] UnitRect  ConvertToDots(const FloatRect& rect) const noexcept;

    [[nodiscard]] UnitPoint ConvertToPixels(const UnitPoint& point) const noexcept;
    [[nodiscard]] UnitPoint ConvertToDots(const UnitPoint& point) const noexcept;
    [[nodiscard]] UnitPoint ConvertToDots(const FramePoint& point_px) const noexcept;

    [[nodiscard]] UnitSize  ConvertToPixels(const UnitSize& size) const noexcept;
    [[nodiscard]] UnitSize  ConvertToDots(const UnitSize& size) const noexcept;
    [[nodiscard]] UnitSize  ConvertToDots(const FrameSize& size_px) const noexcept;

    [[nodiscard]] UnitRect  ConvertToPixels(const UnitRect& rect) const noexcept;
    [[nodiscard]] UnitRect  ConvertToDots(const UnitRect& rect) const noexcept;
    [[nodiscard]] UnitRect  ConvertToDots(const FrameRect& rect_px) const noexcept;

    [[nodiscard]] UnitPoint ConvertToUnits(const FramePoint& point_px, Units units) const noexcept;
    [[nodiscard]] UnitSize  ConvertToUnits(const FrameSize&  size_px,  Units units) const noexcept;
    [[nodiscard]] UnitRect  ConvertToUnits(const FrameRect&  rect_px,  Units units) const noexcept;

    template<typename IntegralT>
    [[nodiscard]] std::enable_if_t<std::is_integral_v<IntegralT>, IntegralT> ConvertPixelsToDots(const IntegralT& pixels) const noexcept
    {
        return static_cast<IntegralT>(std::round(static_cast<float>(pixels) / m_dots_to_pixels_factor));
    }

    template<typename IntegralT>
    [[nodiscard]] std::enable_if_t<std::is_integral_v<IntegralT>, IntegralT> ConvertDotsToPixels(const IntegralT& dots) const noexcept
    {
        return static_cast<IntegralT>(std::round(static_cast<float>(dots) * m_dots_to_pixels_factor));
    }

    template<typename UnitType>
    [[nodiscard]] UnitType ConvertToUnits(const UnitType& value, Units units) const noexcept
    {
        switch(units)
        {
        case Units::Pixels: return ConvertToPixels(value);
        case Units::Dots:   return ConvertToDots(value);
        default:            return UnitType();
        }
    }

    template<typename UnitType>
    [[nodiscard]] bool AreEqual(const UnitType& left, const UnitType& right) const noexcept
    {
        if (left.units == right.units)
            return left == right;
        return left == ConverToUnits(right, left.units);
    }

private:
    gfx::RenderContext&  m_render_context;
    float                m_dots_to_pixels_factor;
    uint32_t             m_font_resolution_dpi;
};

} // namespace Methane::UserInterface
