/******************************************************************************

Copyright 2024 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/SpanComparable.hpp
Comparing operators implementation for std::span type
CREDIT: This implementation is based on https://github.com/brevzin/span_ext

******************************************************************************/
#pragma once

#include <compare>
#include <algorithm>
#include <span>
#include <concepts>
#include <ranges>

namespace Methane::Data::SpanComparable
{
    template<typename T>
    concept StrictComparable = requires(const T& t, const T& u)
    {
        { t < u } -> std::convertible_to<bool>;
        { u < t } -> std::convertible_to<bool>;
    };

    constexpr auto TreeWayCompare = []<StrictComparable T>(const T& t, const T& u)
    {
        if constexpr (std::three_way_comparable<T>)
        {
            return t <=> u;
        } else {
            if (t < u) return std::weak_ordering::less;
            if (u < t) return std::weak_ordering::greater;
            return std::weak_ordering::equivalent;
        }
    };

    template <typename T>
    concept TreeWayCompareInvocable = std::invocable<decltype(TreeWayCompare), T, T>;

    template <typename T, typename U>
    concept SameAs = std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;

    template <typename R, typename T>
    concept ContinousRangeOf = std::ranges::contiguous_range<R const> &&
                               SameAs<T, std::ranges::range_value_t<R const>>;

} // namespace Methane::Data::SpanComparable

namespace std // NOSONAR - namespace is required to extend std
{
    template <equality_comparable T, size_t E,
              Methane::Data::SpanComparable::ContinousRangeOf<T> R>
    constexpr bool operator==(span<T, E> lhs, R const& rhs)
    {
        return ranges::equal(lhs, rhs);
    }

    template <Methane::Data::SpanComparable::TreeWayCompareInvocable T, size_t E,
              Methane::Data::SpanComparable::ContinousRangeOf<T> R>
    constexpr auto operator<=>(span<T, E> lhs, R const& rhs)
    {
        return lexicographical_compare_three_way(
            lhs.begin(), lhs.end(),
            ranges::begin(rhs), ranges::end(rhs),
            Methane::Data::SpanComparable::TreeWayCompare);
    }

} // namespace std
