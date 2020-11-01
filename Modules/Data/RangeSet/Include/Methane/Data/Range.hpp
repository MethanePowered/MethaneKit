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

FILE: Methane/Data/Range.hpp

Range data type representing continuous numeric multitude from start (inclusively)
till end (exclusively): [start, end)

******************************************************************************/

#pragma once

#include <Methane/Checks.hpp>

#include <initializer_list>
#include <algorithm>
#include <sstream>
#include <stdexcept>

#include <Methane/Instrumentation.h>

namespace Methane::Data
{

template<typename ScalarT>
class Range
{
public:
    Range(ScalarT start, ScalarT end) : m_start(start), m_end(end) { META_CHECK_ARG_DESCR(m_start, m_start <= m_end, "range start must be less of equal than end"); }
    Range(std::initializer_list<ScalarT> init) : Range(*init.begin(), *(init.begin() + 1)) { }
    Range(const Range& other) noexcept : m_start(other.m_start), m_end(other.m_end) { }
    Range() noexcept: Range({}, {}) { }
    Range(Range&&) noexcept = default;

    Range<ScalarT>& operator=(const Range<ScalarT>& other) noexcept        { META_FUNCTION_TASK(); m_start = other.m_start; m_end = other.m_end; return *this; }
    bool            operator==(const Range<ScalarT>& other) const noexcept { META_FUNCTION_TASK(); return m_start == other.m_start && m_end == other.m_end; }
    bool            operator!=(const Range<ScalarT>& other) const noexcept { META_FUNCTION_TASK(); return !operator==(other); }
    bool            operator< (const Range<ScalarT>& other) const noexcept { META_FUNCTION_TASK(); return m_end  <= other.m_start; }
    bool            operator> (const Range<ScalarT>& other) const noexcept { META_FUNCTION_TASK(); return m_start > other.end; }

    ScalarT GetStart() const noexcept                           { return m_start; }
    ScalarT GetEnd() const noexcept                             { return m_end; }
    ScalarT GetLength() const noexcept                          { return m_end - m_start; }
    bool    IsEmpty() const noexcept                            { return m_start == m_end; }

    bool    IsAdjacent(const Range& other) const noexcept       { META_FUNCTION_TASK(); return m_start == other.m_end   || other.m_start == m_end; }
    bool    IsOverlapping(const Range& other) const noexcept    { META_FUNCTION_TASK(); return m_start <  other.m_end   && other.m_start <  m_end;  }
    bool    IsMergeable(const Range& other) const noexcept      { META_FUNCTION_TASK(); return m_start <= other.m_end   && other.m_start <= m_end; }
    bool    Contains(const Range& other) const noexcept         { META_FUNCTION_TASK(); return m_start <= other.m_start && other.m_end   <= m_end; }

    Range operator+(const Range& other) const // merge
    {
        META_FUNCTION_TASK();
        META_CHECK_ARG_DESCR(other, IsMergeable(other), "can not merge ranges which are not overlapping or adjacent");
        return Range(std::min(m_start, other.m_start), std::max(m_end, other.m_end));
    }

    Range operator%(const Range& other) const // intersect
    {
        META_FUNCTION_TASK();
        META_CHECK_ARG_DESCR(other, IsMergeable(other), "can not intersect ranges which are not overlapping or adjacent");
        return Range(std::max(m_start, other.m_start), std::min(m_end, other.m_end));
    }

    Range operator-(const Range& other) const // subtract
    {
        META_FUNCTION_TASK();
        META_CHECK_ARG_DESCR(other, IsOverlapping(other), "can not subtract ranges which are not overlapping");
        META_CHECK_ARG_DESCR(other, !Contains(other) && !other.Contains(*this), "can not subtract ranges containing one another");
        return (m_start <= other.m_start) ? Range(m_start, other.m_start) : Range(other.m_end, m_end);
    }

    operator bool() const noexcept
    {
        META_FUNCTION_TASK();
        return !IsEmpty();
    }

    explicit operator std::string() const noexcept
    {
        META_FUNCTION_TASK();
        std::stringstream ss;
        ss << "[" << m_start << ", " << m_end << ")";
        return ss.str();
    }

private:
    ScalarT m_start;
    ScalarT m_end;
};

} // namespace Methane::Data
