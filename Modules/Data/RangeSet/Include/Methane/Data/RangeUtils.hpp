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

FILE: Methane/Data/RangeSet.hpp

Set of ranges with operations of adding and removing a range with maintaining
minimum number of continuous ranges by merging or splitting adjacent ranges in set

******************************************************************************/

#pragma once

#include "RangeSet.hpp"

#include <Methane/Instrumentation.h>

#include <fmt/format.h>
#include <algorithm>

template<typename ScalarT>
struct fmt::formatter<Methane::Data::Range<ScalarT>>
{
    template<typename FormatContext>
    auto format(const Methane::Data::Range<ScalarT>& range, FormatContext& ctx) const
    { return format_to(ctx.out(), "{}", static_cast<std::string>(range)); }

    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const
    { return ctx.end(); }
};

namespace Methane::Data
{

template<typename ScalarT>
Range<ScalarT> ReserveRange(RangeSet<ScalarT>& free_ranges, ScalarT reserved_length) noexcept
{
    typename RangeSet<ScalarT>::ConstIterator free_range_it = std::find_if(
        free_ranges.begin(), free_ranges.end(),
        [reserved_length](const Range<ScalarT>& range)
        {
            return range.GetLength() >= reserved_length;
        }
    );

    if (free_range_it == free_ranges.end())
        return Range<ScalarT>();

    Range<ScalarT> reserved_range(free_range_it->GetStart(), free_range_it->GetStart() + reserved_length);
    free_ranges.Remove(reserved_range);
    return reserved_range;
}

} // namespace Methane::Data
