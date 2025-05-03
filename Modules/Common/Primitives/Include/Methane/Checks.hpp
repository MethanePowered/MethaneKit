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

FILE: Methane/Checks.hpp
Methane short check macroses throwing exceptions on negative check result

  - META_INVALID_ARG_DESCR(argument, description, ...)
  - META_CHECK_NAME[_DESCR](argument_name, condition[, description])
  - META_CHECK[_DESCR](argument, condition[, description])
  - META_CHECK_[TRUE|FALSE][_DESCR](argument[, description])
  - META_CHECK[_NOT]_EQUAL[_DESCR](argument, value, description, ...)
  - META_CHECK_RANGE[_DESCR](argument, range_begin, range_end[, description]) - check in range [begin, end) - end exclusive
  - META_CHECK_RANGE_INC_[_DESCR](argument, range_begin, range_end[, description]) - check in range [begin, end] - end inclusive
  - META_CHECK_LESS[_OR_EQUAL][_DESCR](argument, upper_limit[, description])  - check value <[=] upper_limit
  - META_CHECK_GREATER[_OR_EQUAL][_DESCR](argument, min_value[, description]) - check value >[=] min_value
  - META_CHECK_NOT_EMPTY[_DESCR](argument[, description])
  - META_CHECK_NOT_NULL[_DESCR](argument[, description])
  - META_UNEXPECTED[_DESCR](argument[, description])
  - META_UNEXPECTED_RETURN[_DESCR](argument, return_value[, description])
  - META_FUNCTION_NOT_IMPLEMENTED[_RETURN][_DESCR]([return_value],[description])

******************************************************************************/
#pragma once

#include "Exceptions.hpp"

#ifdef METHANE_CHECKS_ENABLED

#define META_INVALID_ARG_DESCR(argument, description, ...) \
    throw Methane::InvalidArgumentException<std::decay_t<decltype(argument)>>(std::source_location::current(), #argument, argument, fmt::format(description, ## __VA_ARGS__))

#define META_CHECK_DESCR(argument, condition, description, ...) \
    if (!(condition)) \
        throw Methane::InvalidArgumentException<std::decay_t<decltype(argument)>>(std::source_location::current(), #argument, argument, fmt::format(description, ## __VA_ARGS__))

#define META_CHECK(argument, condition) META_CHECK_DESCR(argument, condition, #condition)

#define META_CHECK_NAME_DESCR(argument_name, condition, description, ...) \
    if (!(condition)) \
        throw Methane::InvalidArgumentException<bool>(std::source_location::current(), argument_name, fmt::format(description, ## __VA_ARGS__))

#define META_CHECK_NAME(argument_name, condition) META_CHECK_NAME_DESCR(argument_name, condition, #condition)

#define META_CHECK_TRUE_DESCR(argument, description, ...)  META_CHECK_DESCR(argument, argument, description, ## __VA_ARGS__)
#define META_CHECK_FALSE_DESCR(argument, description, ...) META_CHECK_DESCR(argument, !(argument), description, ## __VA_ARGS__)
#define META_CHECK_EQUAL_DESCR(argument, value, description, ...)  META_CHECK_DESCR(argument, argument == value, description, ## __VA_ARGS__)
#define META_CHECK_NOT_EQUAL_DESCR(argument, value, description, ...) META_CHECK_DESCR(argument, argument != value, description, ## __VA_ARGS__)

#define META_CHECK_TRUE(argument)  META_CHECK_TRUE_DESCR(argument, "")
#define META_CHECK_FALSE(argument) META_CHECK_FALSE_DESCR(argument, "")
#define META_CHECK_EQUAL(argument, value)  META_CHECK_EQUAL_DESCR(argument, value, "")
#define META_CHECK_NOT_EQUAL(argument, value) META_CHECK_NOT_EQUAL_DESCR(argument, value, "")

#define META_CHECK_RANGE_DESCR(argument, range_begin, range_end, description, ...) \
    if (argument < static_cast<std::decay_t<decltype(argument)>>(range_begin) || argument >= static_cast<std::decay_t<decltype(argument)>>(range_end)) \
        throw Methane::OutOfRangeArgumentException<std::decay_t<decltype(argument)>, std::decay_t<decltype(range_begin)>>(std::source_location::current(), #argument, argument, \
                    { range_begin, static_cast<std::decay_t<decltype(range_begin)>>(range_end) }, false, fmt::format(description, ## __VA_ARGS__))

#define META_CHECK_RANGE(argument, range_begin, range_end) META_CHECK_RANGE_DESCR(argument, range_begin, range_end, "")

#define META_CHECK_RANGE_INC_DESCR(argument, range_begin, range_end, description, ...) \
    if (argument < static_cast<std::decay_t<decltype(argument)>>(range_begin) || argument > static_cast<std::decay_t<decltype(argument)>>(range_end)) \
        throw Methane::OutOfRangeArgumentException<std::decay_t<decltype(argument)>, std::decay_t<decltype(range_begin)>>(std::source_location::current(), #argument, argument, \
                    { range_begin, static_cast<std::decay_t<decltype(range_begin)>>(range_end) }, true, fmt::format(description, ## __VA_ARGS__))

#define META_CHECK_INC_RANGE(argument, range_begin, range_end) META_CHECK_RANGE_INC_DESCR(argument, range_begin, range_end, "")

#define META_CHECK_LESS_DESCR(argument, upper_limit, description, ...) \
    if (argument >= static_cast<std::decay_t<decltype(argument)>>(upper_limit)) \
        throw Methane::OutOfRangeArgumentException<std::decay_t<decltype(argument)>, std::decay_t<decltype(upper_limit)>>(std::source_location::current(), #argument, argument, \
                    { std::numeric_limits<std::decay_t<decltype(upper_limit)>>::min(), upper_limit }, false, fmt::format(description, ## __VA_ARGS__))

#define META_CHECK_LESS(argument, upper_limit) META_CHECK_LESS_DESCR(argument, upper_limit, "")

#define META_CHECK_LESS_OR_EQUAL_DESCR(argument, upper_limit, description, ...) \
    if (argument > static_cast<std::decay_t<decltype(argument)>>(upper_limit)) \
        throw Methane::OutOfRangeArgumentException<std::decay_t<decltype(argument)>, std::decay_t<decltype(upper_limit)>>(std::source_location::current(), #argument, argument, \
                    { std::numeric_limits<std::decay_t<decltype(upper_limit)>>::min(), upper_limit }, true, fmt::format(description, ## __VA_ARGS__))

#define META_CHECK_LESS_OR_EQUAL(argument, upper_limit) META_CHECK_LESS_OR_EQUAL_DESCR(argument, upper_limit, "")

#define META_CHECK_GREATER_DESCR(argument, min_value, description, ...) \
    if (argument <= static_cast<std::decay_t<decltype(argument)>>(min_value)) \
        throw Methane::OutOfRangeArgumentException<std::decay_t<decltype(argument)>, std::decay_t<decltype(min_value)>>(std::source_location::current(), #argument, argument, \
                    { min_value, std::numeric_limits<std::decay_t<decltype(min_value)>>::max() }, false, fmt::format(description, ## __VA_ARGS__))

#define META_CHECK_GREATER(argument, upper_limit) META_CHECK_GREATER_DESCR(argument, upper_limit, "")

#define META_CHECK_GREATER_OR_EQUAL_DESCR(argument, min_value, description, ...) \
    if (argument < static_cast<std::decay_t<decltype(argument)>>(min_value)) \
        throw Methane::OutOfRangeArgumentException<std::decay_t<decltype(argument)>, std::decay_t<decltype(min_value)>>(std::source_location::current(), #argument, argument, \
                    { min_value, std::numeric_limits<std::decay_t<decltype(min_value)>>::max() }, true, fmt::format(description, ## __VA_ARGS__))

#define META_CHECK_GREATER_OR_EQUAL(argument, min_value) META_CHECK_GREATER_OR_EQUAL_DESCR(argument, min_value, "")

#define META_CHECK_NOT_EMPTY_DESCR(argument, description, ...) \
    if (argument.empty()) \
        throw Methane::EmptyArgumentException<std::decay_t<decltype(argument)>>(std::source_location::current(), #argument, fmt::format(description, ## __VA_ARGS__))

#define META_CHECK_NOT_EMPTY(argument) META_CHECK_NOT_EMPTY_DESCR(argument, "")

#define META_CHECK_NOT_NULL_DESCR(argument, description, ...) \
    if (!argument) \
        throw Methane::NullPointerArgumentException<std::decay_t<decltype(argument)>>(std::source_location::current(), #argument, fmt::format(description, ## __VA_ARGS__))

#define META_CHECK_NOT_NULL(argument) META_CHECK_NOT_NULL_DESCR(argument, "")

#define META_CHECK_NOT_ZERO_DESCR(argument, description, ...) \
    if (!argument) \
        throw Methane::ZeroArgumentException<std::decay_t<decltype(argument)>>(std::source_location::current(), #argument, fmt::format(description, ## __VA_ARGS__))

#define META_CHECK_NOT_ZERO(argument) META_CHECK_NOT_ZERO_DESCR(argument, "")

#define META_UNEXPECTED_DESCR(argument, description, ...) \
    throw Methane::UnexpectedArgumentException<decltype(argument)>(std::source_location::current(), #argument, argument, fmt::format(description, ## __VA_ARGS__))

#define META_UNEXPECTED(argument) META_UNEXPECTED_DESCR(argument, "")
#define META_UNEXPECTED_RETURN(argument, return_value) META_UNEXPECTED_DESCR(argument, "")
#define META_UNEXPECTED_RETURN_DESCR(argument, return_value, description, ...) META_UNEXPECTED_DESCR(argument, description, ## __VA_ARGS__)

#define META_FUNCTION_NOT_IMPLEMENTED_DESCR(description, ...) \
    throw Methane::NotImplementedException(std::source_location::current(), fmt::format(description, ## __VA_ARGS__))

#define META_FUNCTION_NOT_IMPLEMENTED_RETURN_DESCR(return_value, description, ...) META_FUNCTION_NOT_IMPLEMENTED_DESCR(description, ## __VA_ARGS__)

#define META_FUNCTION_NOT_IMPLEMENTED() META_FUNCTION_NOT_IMPLEMENTED_DESCR("")
#define META_FUNCTION_NOT_IMPLEMENTED_RETURN(return_value) META_FUNCTION_NOT_IMPLEMENTED_RETURN_DESCR(return_value, "")

#else // #ifdef METHANE_CHECKS_ENABLED

// (void)(argument) is added to suppress unused argument warnings
#define META_INVALID_ARG_DESCR(argument, description, ...) (void)(argument)
#define META_CHECK_DESCR(argument, condition, description, ...) (void)(argument)
#define META_CHECK(argument, condition) (void)(argument)
#define META_CHECK_TRUE_DESCR(argument, description, ...) (void)(argument)
#define META_CHECK_TRUE(argument) (void)(argument)
#define META_CHECK_FALSE_DESCR(argument, description, ...) (void)(argument)
#define META_CHECK_FALSE(argument) (void)(argument)
#define META_CHECK_EQUAL_DESCR(argument, value, description, ...) (void)(argument)
#define META_CHECK_EQUAL(argument, value) (void)(argument)
#define META_CHECK_NOT_EQUAL_DESCR(argument, value, description, ...) (void)(argument)
#define META_CHECK_NOT_EQUAL(argument, value) (void)(argument)
#define META_CHECK_NAME_DESCR(argument_name, condition, description, ...)
#define META_CHECK_NAME(argument_name, condition)
#define META_CHECK_RANGE_DESCR(argument, range_begin, range_end, description, ...) (void)(argument)
#define META_CHECK_RANGE(argument, range_begin, range_end) (void)(argument)
#define META_CHECK_RANGE_INC_DESCR(argument, range_begin, range_end, description, ...) (void)(argument)
#define META_CHECK_RANGE_INC(argument, range_begin, range_end) (void)(argument)
#define META_CHECK_LESS_DESCR(argument, upper_limit, description, ...) (void)(argument)
#define META_CHECK_LESS(argument, upper_limit) (void)(argument)
#define META_CHECK_LESS_OR_EQUAL_DESCR(argument, upper_limit, description, ...) (void)(argument)
#define META_CHECK_LESS_OR_EQUAL(argument, upper_limit) (void)(argument)
#define META_CHECK_GREATER_DESCR(argument, min_value, description, ...) (void)(argument)
#define META_CHECK_GREATER(argument, min_value, description, ...) (void)(argument)
#define META_CHECK_GREATER_OR_EQUAL_DESCR(argument, min_value, description, ...) (void)(argument)
#define META_CHECK_GREATER_OR_EQUAL(argument, min_value) (void)(argument)
#define META_CHECK_NOT_EMPTY_DESCR(argument, description, ...) (void)(argument)
#define META_CHECK_NOT_EMPTY(argument) (void)(argument)
#define META_CHECK_NOT_NULL_DESCR(argument, description, ...) (void)(argument)
#define META_CHECK_NOT_NULL(argument) (void)(argument)
#define META_CHECK_NOT_ZERO_DESCR(argument, description, ...) (void)(argument)
#define META_CHECK_NOT_ZERO(argument) (void)(argument)
#define META_UNEXPECTED_DESCR(argument, description, ...) (void)(argument)
#define META_UNEXPECTED(argument) (void)(argument)
#define META_UNEXPECTED_RETURN(argument, return_value) return return_value
#define META_UNEXPECTED_RETURN_DESCR(argument, return_value, description, ...) return return_value
#define META_FUNCTION_NOT_IMPLEMENTED_DESCR(description, ...)
#define META_FUNCTION_NOT_IMPLEMENTED_RETURN_DESCR(return_value, description, ...) return return_value
#define META_FUNCTION_NOT_IMPLEMENTED()
#define META_FUNCTION_NOT_IMPLEMENTED_RETURN(return_value) return return_value

#endif // #ifdef METHANE_CHECKS_ENABLED
