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

  - META_CHECK_ARG[_DESCR](argument_name, condition[, description])
  - META_CHECK_ARG_VALUE[_DESCR](argument, condition[, description])
  - META_CHECK_ARG_IN_RANGE[_DESCR](argument, range_begin, range_end[, description])
  - META_CHECK_ARG_IS_LESS[_DESCR](argument, upper_limit[, description])
  - META_CHECK_ARG_IS_GREATER[_DESCR](argument, min_value[, description])
  - META_CHECK_ARG_NOT_EMPTY[_DESCR](argument[, description])
  - META_CHECK_ARG_NOT_NULL[_DESCR](argument[, description])
  - META_UNEXPECTED_ENUM_ARG[_DESCR](argument[, description])

******************************************************************************/

#include "Exceptions.hpp"

#ifndef __FUNCTION_NAME__
    #ifdef WIN32
        #define __FUNCTION_NAME__   __FUNCTION__
    #else
        #define __FUNCTION_NAME__   __func__
    #endif
#endif

#define META_CHECK_ARG_DESCR(argument_name, condition, description) \
    if (!(condition)) \
        throw Methane::InvalidArgumentException<bool>(__FUNCTION_NAME__, argument_name, description)

#define META_CHECK_ARG(argument_name, condition) META_CHECK_ARG_DESCR(argument_name, condition, #condition)

#define META_CHECK_ARG_VALUE_DESCR(argument, condition, description) \
    if (!(condition)) \
        throw Methane::InvalidArgumentException<decltype(argument)>(__FUNCTION_NAME__, #argument, argument, description)

#define META_CHECK_ARG_VALUE(argument, condition) META_CHECK_ARG_VALUE_DESCR(argument, condition, #condition)

#define META_CHECK_ARG_IN_RANGE_DESCR(argument, range_begin, range_end, description) \
    if (argument < range_begin || argument >= range_end) \
        throw Methane::OutOfRangeArgumentException<decltype(argument)>(__FUNCTION_NAME__, #argument, argument, \
                    { static_cast<decltype(argument)>(range_begin), static_cast<decltype(argument)>(range_end) }, description )

#define META_CHECK_ARG_IN_RANGE(argument, range_begin, range_end) META_CHECK_ARG_IN_RANGE_DESCR(argument, range_begin, range_end, "")

#define META_CHECK_ARG_IS_LESS_DESCR(argument, upper_limit, description) \
    if (argument >= upper_limit) \
        throw Methane::OutOfRangeArgumentException<decltype(argument)>(__FUNCTION_NAME__, #argument, argument, \
                    { std::numeric_limits<decltype(argument)>::min(), static_cast<decltype(argument)>(upper_limit) }, description )

#define META_CHECK_ARG_IS_LESS(argument, upper_limit) META_CHECK_ARG_IS_LESS_DESCR(argument, upper_limit, "")

#define META_CHECK_ARG_IS_GREATER_DESCR(argument, min_value, description) \
    if (argument < min_value) \
        throw Methane::OutOfRangeArgumentException<decltype(argument)>(__FUNCTION_NAME__, #argument, argument, \
                    { static_cast<decltype(argument)>(min_value), std::numeric_limits<decltype(argument)>::max() }, description )

#define META_CHECK_ARG_IS_GREATER(argument, min_value) META_CHECK_ARG_IS_GREATER_DESCR(argument, min_value, "")

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
