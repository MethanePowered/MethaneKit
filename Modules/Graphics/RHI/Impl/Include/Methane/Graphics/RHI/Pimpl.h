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

#if defined METHANE_GFX_DIRECTX

#define METHANE_GFX_API DirectX

#elif defined METHANE_GFX_VULKAN

#define METHANE_GFX_API Vulkan

#elif defined METHANE_GFX_METAL

#define METHANE_GFX_API Metal

#else // METHAN_GFX_[API] is undefined

static_assert(false, "Static graphics API macro-definition is missing.");

#endif

#define META_PIMPL_METHODS_DECLARE(Class) \
    ~Class(); \
    Class(const Class& other); \
    Class(Class&& other) noexcept; \
    Class& operator=(const Class& other); \
    Class& operator=(Class&& other) noexcept

#define META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(Class) \
    Class(); \
    META_PIMPL_METHODS_DECLARE(Class)

#define META_PIMPL_METHODS_COMPARE_DECLARE(Class) \
    bool operator==(const Class& other) const noexcept; \
    bool operator!=(const Class& other) const noexcept; \
    bool operator<(const Class& other) const noexcept

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

#ifdef _DEBUG
// Comment this define to disable checks of pointer to implementation in all PIMPL methods
#define PIMPL_NULL_CHECK_ENABLED
#include <Methane/Checks.hpp>
#endif

#ifdef PIMPL_NULL_CHECK_ENABLED
#define META_PIMPL_NOEXCEPT
#else
#define META_PIMPL_NOEXCEPT noexcept
#endif


