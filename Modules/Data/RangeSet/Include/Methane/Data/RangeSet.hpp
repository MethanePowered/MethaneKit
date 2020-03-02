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

FILE: Methane/Data/RangeSet.hpp

Set of ranges with operations of adding and removing a range with maintaining
minimum number of continuous ranges by merging or splitting adjacent ranges in set

******************************************************************************/

#pragma once

#include "Range.hpp"

#include <Methane/Instrumentation.h>

#include <set>
#include <vector>

namespace Methane::Data
{

template<typename ScalarT>
class RangeSet : protected std::set<Range<ScalarT>>
{
public:
    using BaseSet  = std::set<Range<ScalarT>>;
    using InitList = std::initializer_list<Range<ScalarT>>;
    using Iterator = typename BaseSet::iterator;
    using ConstIterator = typename BaseSet::const_iterator;

    explicit RangeSet() : BaseSet() { ITT_FUNCTION_TASK(); }
    RangeSet(const RangeSet& other) : BaseSet(other) { ITT_FUNCTION_TASK(); }
    RangeSet(InitList init) : BaseSet() { ITT_FUNCTION_TASK(); operator=(init); }

    bool operator==(const RangeSet<ScalarT>& other) const { ITT_FUNCTION_TASK(); return static_cast<const BaseSet&>(*this) == static_cast<const BaseSet&>(other); }
    bool operator==(const BaseSet& other) const           { ITT_FUNCTION_TASK(); return static_cast<const BaseSet&>(*this) == other; }

    RangeSet<ScalarT>& operator=(InitList init)
    {
        ITT_FUNCTION_TASK();
        for (const Range<ScalarT>& range : init)
            Add(range);
        return *this;
    }

    size_t Size() const              { return BaseSet::size();  }
    bool   IsEmpty() const           { return BaseSet::empty(); }
    void   Clear()                   { ITT_FUNCTION_TASK(); BaseSet::clear(); }

    const BaseSet& GetRanges() const { return *this; }
    ConstIterator begin() const      { return BaseSet::begin(); }
    ConstIterator end() const        { return BaseSet::end(); }

    void Add(const Range<ScalarT>& range)
    {
        ITT_FUNCTION_TASK();
        Range<ScalarT> merged_range(range);
        const RangeOfRanges ranges = GetMergeableRanges(range);

        Ranges remove_ranges;
        for (auto range_it = ranges.first; range_it != ranges.second; ++range_it)
        {
            merged_range = merged_range + *range_it;
            remove_ranges.emplace_back(*range_it);
        }

        RemoveRanges(remove_ranges);
        BaseSet::insert(merged_range);
    }

    void Remove(const Range<ScalarT>& range)
    {
        ITT_FUNCTION_TASK();
        Ranges remove_ranges, add_ranges;
        RangeOfRanges ranges = GetMergeableRanges(range);
        for (auto range_it = ranges.first; range_it != ranges.second; ++range_it)
        {
            if (!range.IsOverlapping(*range_it))
                continue;

            remove_ranges.push_back(*range_it);

            if (range.Contains(*range_it))
                continue;
            
            if (range_it->Contains(range))
            {
                const Range<ScalarT> left_sub_range(range_it->GetStart(), range.GetStart());
                if (!left_sub_range.IsEmpty())
                {
                    add_ranges.emplace_back(left_sub_range);
                }

                const Range<ScalarT> right_sub_range(range.GetEnd(), range_it->GetEnd());
                if (!right_sub_range.IsEmpty())
                {
                    add_ranges.emplace_back(right_sub_range);
                }
            }
            else
            {
                Range<ScalarT> trimmed_range = *range_it - range;
                if (!trimmed_range.IsEmpty())
                {
                    add_ranges.emplace_back(trimmed_range);
                }
            }
        }

        RemoveRanges(remove_ranges);
        AddRanges(add_ranges);
    }

protected:
    using RangeOfRanges = std::pair<ConstIterator, ConstIterator>;
    RangeOfRanges GetMergeableRanges(const Range<ScalarT>& range)
    {
        ITT_FUNCTION_TASK();
        if (BaseSet::empty())
        {
            return RangeOfRanges{ BaseSet::end(), BaseSet::end() };
        }

        RangeOfRanges mergeable_ranges = {
            BaseSet::lower_bound(Range<ScalarT>(range.GetStart(), range.GetStart())),
            BaseSet::upper_bound(range)
        };

        if (mergeable_ranges.first != BaseSet::begin())
            mergeable_ranges.first--;

        while (mergeable_ranges.first != BaseSet::end() && !range.IsMergeable(*mergeable_ranges.first))
            mergeable_ranges.first++;

        if (mergeable_ranges.first == BaseSet::end())
            return RangeOfRanges(BaseSet::end(), BaseSet::end());

        while (mergeable_ranges.second != mergeable_ranges.first &&
              (mergeable_ranges.second == BaseSet::end() || !range.IsMergeable(*mergeable_ranges.second)))
        {
            mergeable_ranges.second--;
        }
        mergeable_ranges.second++;

        return mergeable_ranges;
    }

    using Ranges = std::vector<Range<ScalarT>>;
    inline void RemoveRanges(const Ranges& delete_ranges)
    {
        ITT_FUNCTION_TASK();
        for (const Range<ScalarT>& delete_range : delete_ranges)
        {
            BaseSet::erase(delete_range);
        }
    }

    inline void AddRanges(const Ranges& add_ranges)
    {
        ITT_FUNCTION_TASK();
        for(const Range<ScalarT>& add_range : add_ranges)
        {
            BaseSet::insert(add_range);
        }
    }
};

} // namespace Methane::Data
