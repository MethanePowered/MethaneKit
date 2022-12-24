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

#include <memory>

namespace Methane::Graphics
{

// PIMPL implementation is hold using this smart-pointer type
// shared_ptr is used instead of unique_ptr to enable implicit copying of PIMPL types
template<typename ImplType>
using ImplPtr = std::shared_ptr<ImplType>;

} // namespace Methane::Graphics

#ifdef _DEBUG
// Comment this define to disable checks of pointer to implementation in all PIMPL methods
#define PIMPL_NULL_CHECK_ENABLED
#endif

#ifdef PIMPL_NULL_CHECK_ENABLED
#define META_PIMPL_NOEXCEPT
#else
#define META_PIMPL_NOEXCEPT noexcept
#endif

#define META_PIMPL_METHODS_DECLARE(Class) \
    ~Class(); \
    Class(const Class& other); \
    Class(Class&& other) noexcept; \
    Class& operator=(const Class& other); \
    Class& operator=(Class&& other) noexcept; \
    bool operator==(const Class& other) const noexcept; \
    bool operator!=(const Class& other) const noexcept

#define META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(Class) \
    Class(); \
    META_PIMPL_METHODS_DECLARE(Class)

#define META_PIMPL_METHODS_IMPLEMENT(Class) \
    Class::~Class() = default; \
    Class::Class(const Class& other) = default; \
    Class::Class(Class&& other) noexcept = default; \
    Class& Class::operator=(const Class& other) = default; \
    Class& Class::operator=(Class&& other) noexcept = default; \
    bool Class::operator==(const Class& other) const noexcept { return std::addressof(GetInterface()) == std::addressof(other.GetInterface()); } \
    bool Class::operator!=(const Class& other) const noexcept { return !operator==(other); }

#define META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(Class) \
    Class::Class() = default; \
    META_PIMPL_METHODS_IMPLEMENT(Class)
