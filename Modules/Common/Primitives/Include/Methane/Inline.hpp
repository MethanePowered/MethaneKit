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

FILE: Methane/Inline.hpp
Methane cross-compiler inline macro definition.

******************************************************************************/

#pragma once

#if defined(__GNUC__)
#define META_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#define META_INLINE __forceinline
#else
#define META_INLINE inline
#endif