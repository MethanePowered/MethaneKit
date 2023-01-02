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

#include <Methane/Memory.hpp>

#ifdef META_RHI_PIMPL_INLINE

#include <Methane/Inline.hpp>

#define META_RHI_API META_INLINE

#else // META_RHI_PIMPL_INLINE

#define META_RHI_API

#endif // META_RHI_PIMPL_INLINE

#ifdef _DEBUG
// Comment this define to disable checks of pointer to implementation in all PIMPL methods
#define META_PIMPL_NULL_CHECK_ENABLED
#endif

#ifdef META_PIMPL_NULL_CHECK_ENABLED
#define META_PIMPL_NOEXCEPT
#else
#define META_PIMPL_NOEXCEPT noexcept
#endif

#define META_PIMPL_METHODS_DECLARE(Class) \
    META_RHI_API ~Class(); \
    META_RHI_API Class(const Class& other); \
    META_RHI_API Class(Class&& other) noexcept; \
    META_RHI_API Class& operator=(const Class& other); \
    META_RHI_API Class& operator=(Class&& other) noexcept

#define META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(Class) \
    META_RHI_API Class(); \
    META_PIMPL_METHODS_DECLARE(Class)

#define META_PIMPL_METHODS_COMPARE_DECLARE(Class) \
    META_RHI_API bool operator==(const Class& other) const noexcept; \
    META_RHI_API bool operator!=(const Class& other) const noexcept; \
    META_RHI_API bool operator<(const Class& other) const noexcept

#define META_PIMPL_METHODS_IMPLEMENT(Class) \
    Class::~Class() = default; \
    Class::Class(const Class& other) = default; \
    Class::Class(Class&& other) noexcept = default; \
    Class& Class::operator=(const Class& other) = default; \
    Class& Class::operator=(Class&& other) noexcept = default

#define META_PIMPL_METHODS_COMPARE_IMPLEMENT(Class) \
    bool Class::operator==(const Class& other) const noexcept \
    { return (!IsInitialized() && !other.IsInitialized()) ||   \
             (IsInitialized() && other.IsInitialized() && std::addressof(GetInterface()) == std::addressof(other.GetInterface())); } \
    bool Class::operator!=(const Class& other) const noexcept { return !operator==(other); } \
    bool Class::operator<(const Class& other) const noexcept \
    { if  (IsInitialized() && other.IsInitialized()) return std::addressof(GetInterface()) < std::addressof(other.GetInterface()); \
      return !IsInitialized() && other.IsInitialized(); }

#define META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(Class) \
    Class::Class() = default; \
    META_PIMPL_METHODS_IMPLEMENT(Class)

