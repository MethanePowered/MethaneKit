/******************************************************************************

Copyright 2020-2021 Evgeny Gorodetskiy

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
#include "TypeTraits.hpp"

#include <Methane/Graphics/RHI/RenderContext.h>
#include <Methane/Graphics/RHI/RenderPass.h>
#include <Methane/Graphics/RHI/RenderPattern.h>
#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Data/TypeTraits.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Memory.hpp>

namespace rhi = Methane::Graphics::Rhi;
namespace pal = Methane::Platform;

namespace Methane::Platform
{
struct IApp;
}

namespace Methane::UserInterface
{

class Context
{
public:
    Context(const pal::IApp& app, const rhi::CommandQueue& render_cmd_queue, const rhi::RenderPattern& render_pattern);

    const rhi::RenderContext& GetRenderContext() const noexcept      { return m_render_context; }
    const rhi::CommandQueue&  GetRenderCommandQueue() const noexcept { return m_render_cmd_queue; }
    const rhi::RenderPattern& GetRenderPattern() const noexcept      { return m_render_pattern; }

    double           GetDotsToPixelsFactor() const noexcept { return m_dots_to_pixels_factor; }
    uint32_t         GetFontResolutionDpi() const noexcept  { return m_font_resolution_dpi; }
    const FrameSize& GetFrameSize() const noexcept          { return GetRenderContext().GetSettings().frame_size; }

    template<Units units>
    [[nodiscard]] UnitSize  GetFrameSizeIn() const
    {
        META_FUNCTION_TASK();
        if constexpr (units == Units::Pixels)
            return UnitSize(Units::Pixels, GetFrameSize());
        else
            return UnitSize(Units::Dots, GetFrameSize() / m_dots_to_pixels_factor);
    }
    [[nodiscard]] UnitSize  GetFrameSizeInUnits(Units units) const noexcept;

    template<Units units, typename BaseType>
    [[nodiscard]] UnitType<BaseType> ConvertTo(const UnitType<BaseType>& value) const noexcept
    {
        META_FUNCTION_TASK();
        if (value.GetUnits() == units)
            return value;

        if constexpr (units == Units::Pixels)
            return UnitType<BaseType>(Units::Pixels, value * m_dots_to_pixels_factor);
        else
            return UnitType<BaseType>(Units::Dots,   value / m_dots_to_pixels_factor);
    }

    template<Units units>
    [[nodiscard]] UnitPoint ConvertRatioTo(const FloatPoint& point) const noexcept
    {
        META_FUNCTION_TASK();
        return UnitPoint(GetFrameSizeIn<units>() * point);
    }

    template<Units units>
    [[nodiscard]] UnitSize  ConvertRatioTo(const FloatSize& fsize) const noexcept
    {
        META_FUNCTION_TASK();
        return GetFrameSizeIn<units>() * fsize;
    }

    template<Units units>
    UnitRect ConvertRatioTo(const FloatRect& rect) const noexcept
    {
        META_FUNCTION_TASK();
        const UnitSize frame_size_in_units = GetFrameSizeIn<units>();
        const UnitSize origin_in_units = frame_size_in_units * rect.origin;
        return UnitRect(units, FramePoint(origin_in_units.GetWidth(), origin_in_units.GetHeight()), frame_size_in_units * rect.size);
    }

    template<Units units, typename BaseType>
    [[nodiscard]] UnitType<BaseType> ConvertTo(const BaseType& value_px) const noexcept
    {
        META_FUNCTION_TASK();
        if constexpr (units == Units::Pixels)
            return UnitType<BaseType>(Units::Pixels, value_px);
        else
            return UnitType<BaseType>(Units::Dots, value_px / m_dots_to_pixels_factor);
    }

    template<typename ValueType>
    [[nodiscard]] std::conditional_t<TypeTraits<ValueType>::is_unit_type, ValueType, UnitType<ValueType>> ConvertToUnits(const ValueType& value, Units units) const noexcept
    {
        META_FUNCTION_TASK();
        switch(units)
        {
        case Units::Pixels: return ConvertTo<Units::Pixels>(value);
        case Units::Dots:   return ConvertTo<Units::Dots>(value);
        default:            return { };
        }
    }

    template<typename ScalarType>
    [[nodiscard]] std::enable_if_t<std::is_arithmetic_v<ScalarType>, ScalarType> ConvertPixelsToDots(ScalarType pixels) const noexcept
    {
        return Data::RoundCast<ScalarType>(static_cast<decltype(m_dots_to_pixels_factor)>(pixels) / m_dots_to_pixels_factor);
    }

    template<typename ScalarType>
    [[nodiscard]] std::enable_if_t<std::is_arithmetic_v<ScalarType>, ScalarType> ConvertDotsToPixels(ScalarType dots) const noexcept
    {
        return Data::RoundCast<ScalarType>(static_cast<decltype(m_dots_to_pixels_factor)>(dots) * m_dots_to_pixels_factor);
    }

    template<typename BaseType>
    [[nodiscard]] bool AreEqual(const UnitType<BaseType>& left, const UnitType<BaseType>& right) const noexcept
    {
        return left.GetUnits() == right.GetUnits()
             ? left == right
             : left == ConvertToUnits(right, left.GetUnits());
    }

private:
    const rhi::RenderContext m_render_context;
    const rhi::CommandQueue  m_render_cmd_queue;
    const rhi::RenderPattern m_render_pattern;
    double                   m_dots_to_pixels_factor;
    uint32_t                 m_font_resolution_dpi;
};

} // namespace Methane::UserInterface
