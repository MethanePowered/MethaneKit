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

FILE: Methane/Graphics/RectBinPack.hpp
Rectangle bin packing algorithm implementation.

******************************************************************************/

#include <Methane/Data/Rect.hpp>
#include <Methane/Data/Point.hpp>
#include <Methane/Memory.hpp>
#include <Methane/Instrumentation.h>

namespace Methane::Data
{


template<class TRect> // TRect is a template class "Rect<T,D>" defined in "Rect.hpp"
class RectBinPack
{
public:
    using TSize  = typename TRect::Size;
    using TPoint = typename TRect::Point;

    explicit RectBinPack(TSize size, TSize char_margins = TSize())
        : m_root_bin(TRect{ TPoint(), std::move(size) })
        , m_rect_margins(std::move(char_margins))
    {
        META_FUNCTION_TASK();
    }

    const TSize& GetSize() const { return m_root_bin.GetRect().size; }

    // Tries to pack rectangle in free space of rectangular bin
    // returns true is rect is packed and updates rect.origin with coordinates in rectangular bin
    bool TryPack(TRect& rect)
    {
        META_FUNCTION_TASK();
        if (!m_root_bin.TryPack(rect, m_rect_margins))
            return false;

        META_CHECK_ARG_GREATER_OR_EQUAL(rect.GetLeft(), 0);
        META_CHECK_ARG_GREATER_OR_EQUAL(rect.GetTop(), 0);
        META_CHECK_ARG_LESS(rect.GetRight(), m_root_bin.GetRect().size.width + 1);
        META_CHECK_ARG_LESS(rect.GetBottom(), m_root_bin.GetRect().size.height + 1);
        return true;
    }

private:
    class Bin
    {
    public:
        explicit Bin(TRect rect) : m_rect(std::move(rect)) { META_FUNCTION_TASK(); }

        bool         IsEmpty() const noexcept { return !m_small_bin_ptr && !m_large_bin_ptr; }
        const TRect& GetRect() const noexcept { return m_rect; }

        bool TryPack(TRect& rect, const TSize& char_margins)
        {
            META_FUNCTION_TASK();
            if (!rect.size)
                return true;

            if (IsEmpty())
            {
                const TSize char_size_with_margins = rect.size + char_margins;
                if (!(char_size_with_margins <= m_rect.size))
                    return false;

                // Split node rectangle either vertically or horizontally,
                // by creating small rectangle and one big rectangle representing free area not taken by glyph
                const TSize delta = m_rect.size - rect.size;
                if (delta.width < delta.height)
                {
                    // Small top rectangle, to the right of character glyph
                    m_small_bin_ptr = std::make_unique<Bin>(TRect{
                        TPoint(m_rect.origin.GetX() + char_size_with_margins.width, m_rect.origin.GetY()),
                        TSize(m_rect.size.width - char_size_with_margins.width, char_size_with_margins.height)
                    });
                    // Big bottom rectangle, under and to the right of character glyph
                    m_large_bin_ptr = std::make_unique<Bin>(TRect{
                        TPoint(m_rect.origin.GetX(), m_rect.origin.GetY() + char_size_with_margins.height),
                        TSize(m_rect.size.width, m_rect.size.height - char_size_with_margins.height)
                    });
                }
                else
                {
                    // Small left rectangle, under the character glyph
                    m_small_bin_ptr = std::make_unique<Bin>(TRect{
                        TPoint(m_rect.origin.GetX(), m_rect.origin.GetY() + char_size_with_margins.height),
                        TSize(char_size_with_margins.width, m_rect.size.height - char_size_with_margins.height)
                    });
                    // Big right rectangle, to the right and under character glyph
                    m_large_bin_ptr = std::make_unique<Bin>(TRect{
                        TPoint(m_rect.origin.GetX() + char_size_with_margins.width, m_rect.origin.GetY()),
                        TSize(m_rect.size.width - char_size_with_margins.width, m_rect.size.height)
                    });
                }

                rect.origin.SetX(m_rect.origin.GetX());
                rect.origin.SetY(m_rect.origin.GetY());

                return true;
            }

            if (m_small_bin_ptr->TryPack(rect, char_margins))
                return true;

            return m_large_bin_ptr->TryPack(rect, char_margins);
        }

    private:
        const TRect    m_rect;
        UniquePtr<Bin> m_small_bin_ptr;
        UniquePtr<Bin> m_large_bin_ptr;
    };

    Bin         m_root_bin;
    const TSize m_rect_margins;
};

} // namespace Methane::Data
