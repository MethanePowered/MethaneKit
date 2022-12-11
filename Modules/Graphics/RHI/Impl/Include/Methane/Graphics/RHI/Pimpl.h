/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/Pimpl.h
Methane PIMPL common header.

******************************************************************************/

#pragma once

// Comment this define to disable checks of pointer to implementation in all PIMPL methods
#define PIMPL_NULL_CHECK_ENABLED

#ifdef PIMPL_NULL_CHECK_ENABLED
#define META_PIMPL_NOEXCEPT
#else
#define META_PIMPL_NOEXCEPT META_PIMPL_NOEXCEPT
#endif

#define META_PIMPL_METHODS_DECLARE(Class) \
    ~Class(); \
    Class(const Class& other) = delete; \
    Class(Class&& other) noexcept; \
    Class& operator=(const Class& other) = delete; \
    Class& operator=(Class&& other) noexcept

#define META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(Class) \
    Class(); \
    META_PIMPL_METHODS_DECLARE(Class)

#define META_PIMPL_METHODS_IMPLEMENT(Class) \
    Class::~Class() = default; \
    Class::Class(Class&& other) noexcept = default; \
    Class& Class::operator=(Class&& other) noexcept = default

#define META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(Class) \
    Class::Class() = default; \
    META_PIMPL_METHODS_IMPLEMENT(Class)
