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

  - META_CHECK_ARG[_DESCR](argument, condition[, description])
  - META_CHECK_ARG_NAME[_DESCR](argument_name, condition[, description])
  - META_CHECK_ARG_RANGE[_DESCR](argument, range_begin, range_end[, description])
  - META_CHECK_ARG_LESS[_DESCR](argument, upper_limit[, description])
  - META_CHECK_ARG_GREATER_OR_EQUAL[_DESCR](argument, min_value[, description])
  - META_CHECK_ARG_NOT_EMPTY[_DESCR](argument[, description])
  - META_CHECK_ARG_NOT_NULL[_DESCR](argument[, description])
  - META_UNEXPECTED_ENUM_ARG[_DESCR](argument[, description])
  - META_FUNCTION_NOT_IMPLEMENTED[_DESCR]([description])

******************************************************************************/
#pragma once

#include "Exceptions.hpp"

#ifdef METHANE_CHECKS_ENABLED

#ifndef __FUNCTION_NAME__
    #ifdef WIN32
        #define __FUNCTION_NAME__ __FUNCTION__
    #else
        #define __FUNCTION_NAME__ __func__
    #endif
#endif

#define META_CHECK_ARG_DESCR(argument, condition, description) \
    if (!(condition)) \
        throw Methane::InvalidArgumentException<decltype(argument)>(__FUNCTION_NAME__, #argument, argument, description)

#define META_CHECK_ARG(argument, condition) META_CHECK_ARG_DESCR(argument, condition, #condition)

#define META_CHECK_ARG_NAME_DESCR(argument_name, condition, description) \
    if (!(condition)) \
        throw Methane::InvalidArgumentException<bool>(__FUNCTION_NAME__, argument_name, description)

#define META_CHECK_ARG_NAME(argument_name, condition) META_CHECK_ARG_NAME_DESCR(argument_name, condition, #condition)

#define META_CHECK_ARG_RANGE_DESCR(argument, range_begin, range_end, description) \
    if (argument < static_cast<std::decay<typename std::decay<decltype(argument)>::type>(range_begin) || argument >= static_cast<typename std::decay<decltype(argument)>::type>(range_end)) \
        throw Methane::OutOfRangeArgumentException<typename std::decay<decltype(argument)>::type, typename std::decay<decltype(range_begin)>::type>(__FUNCTION_NAME__, #argument, argument, \
                    { range_begin, static_cast<typename std::decay<decltype(range_begin)>::type>(range_end) }, description )

#define META_CHECK_ARG_RANGE(argument, range_begin, range_end) META_CHECK_ARG_RANGE_DESCR(argument, range_begin, range_end, "")

#define META_CHECK_ARG_LESS_DESCR(argument, upper_limit, description) \
    if (argument >= static_cast<typename std::decay<decltype(argument)>::type>(upper_limit)) \
        throw Methane::OutOfRangeArgumentException<typename std::decay<decltype(argument)>::type, typename std::decay<decltype(upper_limit)>::type>(__FUNCTION_NAME__, #argument, argument, \
                    { std::numeric_limits<typename std::decay<decltype(upper_limit)>::type>::min(), upper_limit }, description )

#define META_CHECK_ARG_LESS(argument, upper_limit) META_CHECK_ARG_LESS_DESCR(argument, upper_limit, "")

#define META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(argument, min_value, description) \
    if (argument < static_cast<typename std::decay<decltype(argument)>::type>(min_value)) \
        throw Methane::OutOfRangeArgumentException<typename std::decay<decltype(argument)>::type, typename std::decay<decltype(min_value)>::type>(__FUNCTION_NAME__, #argument, argument, \
                    { min_value, std::numeric_limits<typename std::decay<decltype(min_value)>::type>::max() }, description )

#define META_CHECK_ARG_GREATER_OR_EQUAL(argument, min_value) META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(argument, min_value, "")

#define META_CHECK_ARG_NOT_EMPTY_DESCR(argument, description) \
    if (argument.empty()) \
        throw Methane::EmptyArgumentException<decltype(argument)>(__FUNCTION_NAME__, #argument, description)

#define META_CHECK_ARG_NOT_EMPTY(argument) META_CHECK_ARG_NOT_EMPTY_DESCR(argument, "")

#define META_CHECK_ARG_NOT_NULL_DESCR(argument, description) \
    if (!argument) \
        throw Methane::NullPointerArgumentException<decltype(argument)>(__FUNCTION_NAME__, #argument, description)

#define META_CHECK_ARG_NOT_NULL(argument) META_CHECK_ARG_NOT_NULL_DESCR(argument, "")

#define META_CHECK_ARG_NOT_ZERO_DESCR(argument, description) \
    if (!argument) \
        throw Methane::ZeroArgumentException<decltype(argument)>(__FUNCTION_NAME__, #argument, description)

#define META_CHECK_ARG_NOT_ZERO(argument) META_CHECK_ARG_NOT_ZERO_DESCR(argument, "")

#define META_UNEXPECTED_ENUM_ARG_DESCR(argument, description) \
    throw Methane::UnexpectedEnumArgumentException<decltype(argument)>(__FUNCTION_NAME__, #argument, argument, description)

#define META_UNEXPECTED_ENUM_ARG(argument) META_UNEXPECTED_ENUM_ARG_DESCR(argument, "")

#define META_FUNCTION_NOT_IMPLEMENTED_DESCR(description) \
    throw Methane::NotImplementedException(__FUNCTION_NAME__, description)

#define META_FUNCTION_NOT_IMPLEMENTED() META_FUNCTION_NOT_IMPLEMENTED_DESCR("")

#else // #ifdef METHANE_CHECKS_ENABLED

// (void)argument is added to suppress unused argument warnings
#define META_CHECK_ARG_DESCR(argument, condition, description) (void)argument
#define META_CHECK_ARG(argument, condition) (void)argument
#define META_CHECK_ARG_NAME_DESCR(argument_name, condition, description)
#define META_CHECK_ARG_NAME(argument_name, condition)
#define META_CHECK_ARG_RANGE_DESCR(argument, range_begin, range_end, description) (void)argument
#define META_CHECK_ARG_RANGE(argument, range_begin, range_end) (void)argument
#define META_CHECK_ARG_LESS_DESCR(argument, upper_limit, description) (void)argument
#define META_CHECK_ARG_LESS(argument, upper_limit) (void)argument
#define META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(argument, min_value, description) (void)argument
#define META_CHECK_ARG_GREATER_OR_EQUAL(argument, min_value) (void)argument
#define META_CHECK_ARG_NOT_EMPTY_DESCR(argument, description) (void)argument
#define META_CHECK_ARG_NOT_EMPTY(argument) (void)argument
#define META_CHECK_ARG_NOT_NULL_DESCR(argument, description) (void)argument
#define META_CHECK_ARG_NOT_NULL(argument) (void)argument
#define META_CHECK_ARG_NOT_ZERO_DESCR(argument, description) (void)argument
#define META_CHECK_ARG_NOT_ZERO(argument) (void)argument
#define META_UNEXPECTED_ENUM_ARG_DESCR(argument, description) (void)argument
#define META_UNEXPECTED_ENUM_ARG(argument) (void)argument
#define META_FUNCTION_NOT_IMPLEMENTED_DESCR(description)
#define META_FUNCTION_NOT_IMPLEMENTED()

#endif // #ifdef METHANE_CHECKS_ENABLED